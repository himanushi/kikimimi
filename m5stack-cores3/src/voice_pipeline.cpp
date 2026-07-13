#include "voice_pipeline.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <M5Unified.h>
#include <NetworkClientSecure.h>

#include <algorithm>
#include <cstring>

#include "pure/chat_protocol.h"
#include "pure/multipart_form.h"
#include "pure/usage_cost.h"
#include "pure/vad.h"
#include "pure/wav.h"

namespace {

constexpr const char* API_HOST = "https://api.openai.com";
constexpr const char* STT_MODEL = "gpt-4o-mini-transcribe";
constexpr const char* CHAT_MODEL = "gpt-5-nano";
constexpr const char* TTS_MODEL = "gpt-4o-mini-tts";
constexpr const char* TTS_VOICE = "marin";  // realtime_client と同じ声(00003 で確認済み)

// 録音(STT 送信用): 16kHz mono 16bit
constexpr uint32_t REC_SAMPLE_RATE = 16000;
constexpr uint32_t REC_MAX_SECONDS = 15;
constexpr size_t REC_MAX_SAMPLES = REC_SAMPLE_RATE * REC_MAX_SECONDS;
constexpr uint32_t REC_CHUNK_MS = 100;
constexpr size_t REC_CHUNK_SAMPLES = REC_SAMPLE_RATE * REC_CHUNK_MS / 1000;

// STT へ送る multipart body 用のバッファ(WAV 本体 + 前後の境界文字列の余裕)
constexpr size_t STT_BODY_SLACK_BYTES = 1024;
constexpr size_t STT_BODY_MAX_BYTES = WAV_HEADER_SIZE + REC_MAX_SAMPLES * 2 + STT_BODY_SLACK_BYTES;

// TTS 応答(再生用): 24kHz mono 16bit、30 秒上限(realtime_client と同じ上限を踏襲)
constexpr uint32_t TTS_SAMPLE_RATE = 24000;
constexpr uint32_t TTS_MAX_SECONDS = 30;
constexpr size_t TTS_MAX_BYTES = TTS_SAMPLE_RATE * TTS_MAX_SECONDS * sizeof(int16_t);

// エネルギー VAD の閾値。マイクゲイン依存のため実機での調整が必要になる可能性がある
// (knowhow: pio-energy-vad-threshold-needs-device-tuning.md)
constexpr vad::Config VAD_CONFIG{/*startThreshold=*/900, /*minSpeechMs=*/200, /*hangoverMs=*/700};

// デフォルトの 5 秒では STT(音声アップロード)・TTS(長文合成)が途中で切れうるため、
// 各段ごとに余裕を持ったタイムアウトを明示する
constexpr uint32_t STT_TIMEOUT_MS = 15000;
constexpr uint32_t CHAT_TIMEOUT_MS = 20000;
constexpr uint32_t TTS_TIMEOUT_MS = 20000;

VoicePipelineState state = VoicePipelineState::Idle;
VoicePipelineCallbacks callbacks;
String pendingApiKey;
String pendingInstructions;
chat_protocol::History conversationHistory;  // セッション中のみ保持(voicePipelineStart でクリア)

uint8_t speakerVolume = 128;

uint8_t* recBuf = nullptr;      // WAV_HEADER_SIZE + REC_MAX_SAMPLES*2(先頭 44 byte はヘッダ用に予約)
uint32_t recSamples = 0;
int16_t* recChunkScratch = nullptr;  // 1 チャンク分の録音スクラッチ(ピーク計算・VAD 用)
uint8_t* sttBodyBuf = nullptr;  // STT multipart body の組み立て先
uint8_t* ttsBuf = nullptr;      // TTS 応答 PCM の受信先
size_t ttsLen = 0;

vad::State vadState;
double sessionCostJpy = 0.0;

bool ensureBuffersAllocated() {
    if (!recBuf) recBuf = static_cast<uint8_t*>(heap_caps_malloc(WAV_HEADER_SIZE + REC_MAX_SAMPLES * 2, MALLOC_CAP_SPIRAM));
    if (!recChunkScratch) recChunkScratch = static_cast<int16_t*>(heap_caps_malloc(REC_CHUNK_SAMPLES * sizeof(int16_t), MALLOC_CAP_SPIRAM));
    if (!sttBodyBuf) sttBodyBuf = static_cast<uint8_t*>(heap_caps_malloc(STT_BODY_MAX_BYTES, MALLOC_CAP_SPIRAM));
    if (!ttsBuf) ttsBuf = static_cast<uint8_t*>(heap_caps_malloc(TTS_MAX_BYTES, MALLOC_CAP_SPIRAM));
    return recBuf && recChunkScratch && sttBodyBuf && ttsBuf;
}

void setState(VoicePipelineState next) {
    state = next;
    if (callbacks.onStateChanged) callbacks.onStateChanged(next);
}

int16_t peakAbs(const int16_t* samples, size_t count) {
    int16_t peak = 0;
    for (size_t i = 0; i < count; ++i) {
        int16_t v = samples[i] < 0 ? -samples[i] : samples[i];
        if (v > peak) peak = v;
    }
    return peak;
}

void resetRecordingState() {
    recSamples = 0;
    vadState = vad::State{};
}

void startListening() {
    M5.Speaker.end();
    M5.Mic.begin();
    resetRecordingState();
    setState(VoicePipelineState::Listening);
}

// https:// 固定・証明書検証なし(個人用途の単一デバイス。realtime_client の WebSocket 接続と
// 同じ信頼モデルに合わせる)。同時に 1 リクエストしか走らない前提の static 共有クライアント
bool beginHttp(HTTPClient& http, const String& path) {
    static NetworkClientSecure client;
    client.setInsecure();
    return http.begin(client, String(API_HOST) + path);
}

// error.message を含む JSON なら取り出し、失敗すれば生ボディの先頭を返す(空なら httpCode のみ)
String extractErrorMessage(const String& body, int httpCode) {
    JsonDocument doc;
    if (!deserializeJson(doc, body)) {
        const char* msg = doc["error"]["message"] | "";
        if (strlen(msg) > 0) return String(msg);
    }
    return "HTTP " + String(httpCode);
}

struct SttResult {
    bool ok = false;
    String transcript;
    long inputAudioTokens = 0;
    long outputTextTokens = 0;
    String errorMessage;
};

SttResult callSttApi(const String& apiKey, uint32_t dataBytes) {
    SttResult result;
    wavWriteHeader(recBuf, REC_SAMPLE_RATE, 16, 1, dataBytes);
    size_t wavLen = WAV_HEADER_SIZE + dataBytes;

    String boundary = "kikimimiB" + String(millis()) + String(random(1000, 9999));
    auto parts = multipart_form::buildFileUploadParts(
        boundary.c_str(), {{"model", STT_MODEL}, {"language", "ja"}}, "file", "audio.wav", "audio/wav");
    size_t bodyLen = parts.preamble.size() + wavLen + parts.epilogue.size();
    if (bodyLen > STT_BODY_MAX_BYTES) {
        result.errorMessage = "録音が長すぎます";
        return result;
    }
    memcpy(sttBodyBuf, parts.preamble.data(), parts.preamble.size());
    memcpy(sttBodyBuf + parts.preamble.size(), recBuf, wavLen);
    memcpy(sttBodyBuf + parts.preamble.size() + wavLen, parts.epilogue.data(), parts.epilogue.size());

    HTTPClient http;
    http.setTimeout(STT_TIMEOUT_MS);
    if (!beginHttp(http, "/v1/audio/transcriptions")) {
        result.errorMessage = "接続できませんでした";
        return result;
    }
    http.addHeader("Authorization", "Bearer " + apiKey);
    http.addHeader("Content-Type", multipart_form::contentTypeHeader(boundary.c_str()).c_str());

    int httpCode = http.POST(sttBodyBuf, bodyLen);
    String body = httpCode > 0 ? http.getString() : String();
    http.end();
    if (httpCode != 200) {
        result.errorMessage = httpCode > 0 ? extractErrorMessage(body, httpCode) : "通信エラー";
        return result;
    }

    JsonDocument doc;
    if (deserializeJson(doc, body)) {
        result.errorMessage = "音声認識の応答を解釈できませんでした";
        return result;
    }
    result.ok = true;
    result.transcript = String(doc["text"] | "");
    JsonObjectConst inputDetails = doc["usage"]["input_token_details"];
    result.inputAudioTokens = inputDetails["audio_tokens"] | (doc["usage"]["input_tokens"] | 0L);
    result.outputTextTokens = doc["usage"]["output_tokens"] | 0L;
    return result;
}

struct ChatResult {
    bool ok = false;
    String content;
    chat_protocol::Usage usage;
    String errorMessage;
};

ChatResult callChatApi(const String& apiKey, const String& instructions, const String& userText) {
    ChatResult result;
    std::string json = chat_protocol::buildChatRequest(CHAT_MODEL, instructions.c_str(),
                                                        conversationHistory, userText.c_str());

    HTTPClient http;
    http.setTimeout(CHAT_TIMEOUT_MS);
    if (!beginHttp(http, "/v1/chat/completions")) {
        result.errorMessage = "接続できませんでした";
        return result;
    }
    http.addHeader("Authorization", "Bearer " + apiKey);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST(String(json.c_str()));
    String body = httpCode > 0 ? http.getString() : String();
    http.end();
    if (httpCode != 200) {
        result.errorMessage = httpCode > 0 ? extractErrorMessage(body, httpCode) : "通信エラー";
        return result;
    }

    chat_protocol::ChatResponse parsed = chat_protocol::parseChatResponse(body.c_str());
    if (!parsed.ok) {
        result.errorMessage = "応答を解釈できませんでした";
        return result;
    }
    result.ok = true;
    result.content = String(parsed.content.c_str());
    result.usage = parsed.usage;
    return result;
}

// TTS 応答は chunked + keep-alive で返るため、生ストリームの自前読みでは終端を検知できず
// (connected() が落ちない)、chunked のフレーミングバイトも混入する。chunked のデコードと
// 終端検知を行う HTTPClient::writeToStream() に、バッファへ書くだけの Stream を渡して受ける
class PcmBufferStream : public Stream {
public:
    size_t write(const uint8_t* data, size_t len) override {
        size_t space = TTS_MAX_BYTES - received;
        size_t n = std::min(len, space);
        memcpy(ttsBuf + received, data, n);
        received += n;
        return len;  // 30 秒上限の超過分は捨てるが、消費済みと返して転送は最後まで続けさせる
    }
    size_t write(uint8_t b) override { return write(&b, 1); }
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
    size_t received = 0;
};

struct TtsResult {
    bool ok = false;
    String errorMessage;
};

TtsResult callTtsApi(const String& apiKey, const String& text) {
    TtsResult result;
    JsonDocument doc;
    doc["model"] = TTS_MODEL;
    doc["input"] = text.c_str();
    doc["voice"] = TTS_VOICE;
    doc["response_format"] = "pcm";
    String json;
    serializeJson(doc, json);

    HTTPClient http;
    http.setTimeout(TTS_TIMEOUT_MS);
    if (!beginHttp(http, "/v1/audio/speech")) {
        result.errorMessage = "接続できませんでした";
        return result;
    }
    http.addHeader("Authorization", "Bearer " + apiKey);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST(json);
    if (httpCode != 200) {
        String body = httpCode > 0 ? http.getString() : String();
        http.end();
        result.errorMessage = httpCode > 0 ? extractErrorMessage(body, httpCode) : "通信エラー";
        return result;
    }
    PcmBufferStream pcm;
    int written = http.writeToStream(&pcm);
    http.end();
    if (written < 0) {
        result.errorMessage = "音声合成の受信に失敗しました(code " + String(written) + ")";
        return result;
    }
    if (pcm.received == 0) {
        result.errorMessage = "音声合成の応答が空でした";
        return result;
    }
    ttsLen = pcm.received;
    result.ok = true;
    return result;
}

void reportError(const String& message) {
    M5.Mic.end();
    M5.Speaker.end();
    setState(VoicePipelineState::ErrorState);
    if (callbacks.onError) callbacks.onError(message);
}

// Thinking: STT → chat → TTS を順に呼ぶ。どこかで失敗したら ErrorState にして抜ける
void runPipeline() {
    uint32_t t0 = millis();
    SttResult stt = callSttApi(pendingApiKey, recSamples * 2);
    Serial.printf("[pipeline] stt %s %lums rec=%luB text=%dchars\n", stt.ok ? "ok" : "FAIL",
                  millis() - t0, recSamples * 2, stt.transcript.length());
    if (!stt.ok) {
        Serial.printf("[pipeline] stt error: %s\n", stt.errorMessage.c_str());
        reportError("音声認識に失敗しました: " + stt.errorMessage);
        return;
    }
    if (stt.transcript.length() == 0) {
        // VAD の誤検知(ノイズ)等で無音相当の書き起こしになった場合は、聞き直しへ戻る
        startListening();
        return;
    }
    if (callbacks.onTranscriptUpdated) {
        callbacks.onTranscriptUpdated(transcript_view::Speaker::User, stt.transcript, true);
    }

    uint32_t t1 = millis();
    ChatResult chat = callChatApi(pendingApiKey, pendingInstructions, stt.transcript);
    Serial.printf("[pipeline] chat %s %lums out=%dchars\n", chat.ok ? "ok" : "FAIL", millis() - t1,
                  chat.content.length());
    if (!chat.ok) {
        Serial.printf("[pipeline] chat error: %s\n", chat.errorMessage.c_str());
        reportError("応答生成に失敗しました: " + chat.errorMessage);
        return;
    }
    if (callbacks.onTranscriptUpdated) {
        callbacks.onTranscriptUpdated(transcript_view::Speaker::Assistant, chat.content, true);
    }
    conversationHistory.push_back({"user", stt.transcript.c_str()});
    conversationHistory.push_back({"assistant", chat.content.c_str()});

    uint32_t t2 = millis();
    TtsResult tts = callTtsApi(pendingApiKey, chat.content);
    Serial.printf("[pipeline] tts %s %lums pcm=%uB total=%lums\n", tts.ok ? "ok" : "FAIL",
                  millis() - t2, ttsLen, millis() - t0);
    if (!tts.ok) {
        Serial.printf("[pipeline] tts error: %s\n", tts.errorMessage.c_str());
        reportError("音声合成に失敗しました: " + tts.errorMessage);
        return;
    }

    double sttCost = usage_cost::calculateSttCostJpy(stt.inputAudioTokens, stt.outputTextTokens);
    double chatCost = usage_cost::calculateChatCostJpy(chat.usage.promptTokens, chat.usage.completionTokens,
                                                       chat.usage.cachedTokens);
    double ttsCost = usage_cost::estimateTtsCostJpy(chat.content.length(), ttsLen, TTS_SAMPLE_RATE);
    sessionCostJpy += sttCost + chatCost + ttsCost;
    if (callbacks.onCostUpdated) callbacks.onCostUpdated(sessionCostJpy);

    M5.Speaker.begin();
    M5.Speaker.setVolume(speakerVolume);
    setState(VoicePipelineState::Speaking);
    M5.Speaker.playRaw(reinterpret_cast<int16_t*>(ttsBuf), ttsLen / sizeof(int16_t), TTS_SAMPLE_RATE, false, 1, 0);
}

}  // namespace

void voicePipelineSetSpeakerVolume(uint8_t volume) {
    speakerVolume = volume;
}

void voicePipelineStart(const String& apiKey, const String& instructions, VoicePipelineCallbacks cb) {
    callbacks = cb;
    if (apiKey.length() == 0) {
        setState(VoicePipelineState::ErrorState);
        if (callbacks.onError) callbacks.onError("API キーが未登録です");
        return;
    }
    if (!ensureBuffersAllocated()) {
        setState(VoicePipelineState::ErrorState);
        if (callbacks.onError) callbacks.onError("音声バッファの確保に失敗しました");
        return;
    }
    pendingApiKey = apiKey;
    pendingInstructions = instructions;
    conversationHistory.clear();
    sessionCostJpy = 0.0;
    if (callbacks.onCostUpdated) callbacks.onCostUpdated(sessionCostJpy);
    startListening();
}

void voicePipelineStop() {
    M5.Mic.end();
    M5.Speaker.end();
    setState(VoicePipelineState::Idle);
}

void voicePipelineLoop() {
    if (state == VoicePipelineState::Idle || state == VoicePipelineState::ErrorState) return;

    if (state == VoicePipelineState::Listening && recChunkScratch) {
        // record は直前チャンクの取り込み完了を待ってからキューするため連続録音になる
        if (M5.Mic.record(recChunkScratch, REC_CHUNK_SAMPLES, REC_SAMPLE_RATE)) {
            bool hasRoom = recSamples + REC_CHUNK_SAMPLES <= REC_MAX_SAMPLES;
            if (hasRoom) {
                int16_t* dst = reinterpret_cast<int16_t*>(recBuf + WAV_HEADER_SIZE) + recSamples;
                memcpy(dst, recChunkScratch, REC_CHUNK_SAMPLES * sizeof(int16_t));
                recSamples += REC_CHUNK_SAMPLES;
            }
            int16_t peak = peakAbs(recChunkScratch, REC_CHUNK_SAMPLES);
            vadState = vad::processChunk(vadState, peak, REC_CHUNK_MS, VAD_CONFIG);

            bool bufferFull = !hasRoom;
            if (vadState.phase == vad::Phase::Ended || (bufferFull && vadState.phase == vad::Phase::Speaking)) {
                while (M5.Mic.isRecording()) delay(10);
                M5.Mic.end();
                setState(VoicePipelineState::Thinking);
                runPipeline();
            } else if (bufferFull) {
                // 発話が確定しないまま録音上限に達した(ずっと無音・ノイズのみ): 聞き直す
                resetRecordingState();
            }
        }
    } else if (state == VoicePipelineState::Speaking) {
        if (M5.Speaker.isPlaying(0) == 0) {
            M5.Speaker.end();
            startListening();
        }
    }
}

VoicePipelineState voicePipelineCurrentState() {
    return state;
}
