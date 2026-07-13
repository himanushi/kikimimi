#include "realtime_client.h"

#include <M5Unified.h>
#include <WebSocketsClient.h>

#include <algorithm>
#include <cstring>

#include "pure/audio_codec.h"
#include "pure/realtime_protocol.h"
#include "pure/usage_cost.h"

namespace {

constexpr const char* REALTIME_HOST = "api.openai.com";
constexpr uint16_t REALTIME_PORT = 443;
constexpr const char* REALTIME_PATH = "/v1/realtime?model=gpt-realtime";

// Realtime API の音声フォーマット(session.update と揃える)
constexpr uint32_t AUDIO_SAMPLE_RATE = 24000;
constexpr uint32_t CHUNK_MS = 100;
constexpr size_t CHUNK_SAMPLES = AUDIO_SAMPLE_RATE * CHUNK_MS / 1000;

// 応答音声はターンごとに全量受信してから再生する(PSRAM に貯める)。
// 30 秒を超える応答は想定しないため、超過分は切り捨てる
constexpr size_t PLAYBACK_MAX_SAMPLES = AUDIO_SAMPLE_RATE * 30;
constexpr size_t PLAYBACK_MAX_BYTES = PLAYBACK_MAX_SAMPLES * sizeof(int16_t);

WebSocketsClient ws;
RealtimeState state = RealtimeState::Idle;
RealtimeCallbacks callbacks;
String pendingInstructions;
// realtimeDisconnect() による能動的切断を、WStype_DISCONNECTED/ERROR がその場で同期的に
// 発火した場合にも「エラーではない」と区別するためのフラグ
bool disconnectRequested = false;

int16_t* recordChunkBuf = nullptr;  // CHUNK_SAMPLES 分の録音スクラッチ(PSRAM)
uint8_t* playbackBuf = nullptr;     // 応答音声の受信バッファ(PSRAM)
size_t playbackLen = 0;             // 今回の応答でこれまでに貯めたバイト数
double sessionCostJpy = 0.0;        // このセッション(realtimeConnect 以降)の累計金額

bool ensureBuffersAllocated() {
    if (!recordChunkBuf) {
        recordChunkBuf =
            static_cast<int16_t*>(heap_caps_malloc(CHUNK_SAMPLES * sizeof(int16_t), MALLOC_CAP_SPIRAM));
    }
    if (!playbackBuf) {
        playbackBuf = static_cast<uint8_t*>(heap_caps_malloc(PLAYBACK_MAX_BYTES, MALLOC_CAP_SPIRAM));
    }
    return recordChunkBuf != nullptr && playbackBuf != nullptr;
}

void setState(RealtimeState next) {
    state = next;
    if (callbacks.onStateChanged) callbacks.onStateChanged(next);
}

// Mic を開始して録音・送信フェーズへ(Speaker とは排他のため先に止める)
void startListening() {
    M5.Speaker.end();
    M5.Mic.begin();
    setState(RealtimeState::Listening);
}

// 発話終了を検知したら録音を止め、応答音声を待つ
void stopListeningForThinking() {
    while (M5.Mic.isRecording()) delay(10);
    M5.Mic.end();
    playbackLen = 0;
    setState(RealtimeState::Thinking);
}

// 応答音声が貯まったら Mic を止めたまま Speaker で再生する
void beginSpeaking() {
    if (playbackLen == 0 || !playbackBuf) {
        // 音声を伴わない応答(想定外だが、聞き続けられるよう Listening に戻す)
        startListening();
        return;
    }
    M5.Speaker.begin();
    setState(RealtimeState::Speaking);
    M5.Speaker.playRaw(reinterpret_cast<int16_t*>(playbackBuf), playbackLen / sizeof(int16_t),
                       AUDIO_SAMPLE_RATE, false, 1, 0);
}

bool wasConversing() {
    return state == RealtimeState::Listening || state == RealtimeState::Thinking ||
           state == RealtimeState::Speaking;
}

// 確立前の切断・エラーをまとめて処理する(WStype_DISCONNECTED/WStype_ERROR 共通)
void handleConnectionFailure() {
    if (disconnectRequested) return;  // 能動的切断はエラー扱いしない
    if (state == RealtimeState::Idle) return;
    bool wasActive = wasConversing();
    M5.Mic.end();
    M5.Speaker.end();
    setState(RealtimeState::ErrorState);
    if (!wasActive && callbacks.onError) {
        callbacks.onError("接続できませんでした(API キーを確認してください)");
    }
}

void handleFrame(const char* payload) {
    realtime_protocol::ServerEvent ev = realtime_protocol::parseServerEvent(payload);
    switch (ev.type) {
        case realtime_protocol::ServerEventType::SessionCreated: {
            Serial.println("[realtime] session.created");
            std::string json =
                realtime_protocol::buildSessionUpdateEvent(pendingInstructions.c_str());
            ws.sendTXT(json.c_str());
            break;
        }
        case realtime_protocol::ServerEventType::SessionUpdated:
            Serial.println("[realtime] session.updated");
            startListening();
            break;
        case realtime_protocol::ServerEventType::ResponseOutputAudioDelta: {
            auto bytes = audio_codec::decodeBase64(ev.audioDelta);
            if (!bytes.empty() && playbackBuf) {
                size_t space = PLAYBACK_MAX_BYTES - playbackLen;
                size_t n = std::min(bytes.size(), space);
                if (n > 0) {
                    memcpy(playbackBuf + playbackLen, bytes.data(), n);
                    playbackLen += n;
                }
            }
            break;
        }
        case realtime_protocol::ServerEventType::SpeechStarted:
            Serial.println("[realtime] speech_started");
            break;
        case realtime_protocol::ServerEventType::SpeechStopped:
            Serial.println("[realtime] speech_stopped");
            if (state == RealtimeState::Listening) stopListeningForThinking();
            break;
        case realtime_protocol::ServerEventType::ResponseDone:
            Serial.println("[realtime] response.done");
            sessionCostJpy += usage_cost::calculateCostJpy(ev.usage);
            if (callbacks.onCostUpdated) callbacks.onCostUpdated(sessionCostJpy);
            if (state == RealtimeState::Thinking) beginSpeaking();
            break;
        case realtime_protocol::ServerEventType::Error:
            Serial.printf("[realtime] error: %s\n", ev.errorMessage.c_str());
            if (callbacks.onError) callbacks.onError(ev.errorMessage.c_str());
            M5.Mic.end();
            M5.Speaker.end();
            setState(RealtimeState::ErrorState);
            break;
        case realtime_protocol::ServerEventType::ResponseOutputTextDelta:
        case realtime_protocol::ServerEventType::Unknown:
            break;
    }
}

void onWsEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED:
            Serial.println("[realtime] ws connected");
            break;
        case WStype_TEXT:
            // 音声 delta のフレームサイズ実測用(旧上限 15KB 超の頻度を知るため大きいものだけ)
            if (length > 8 * 1024) {
                Serial.printf("[realtime] rx large frame %u bytes heap=%u\n",
                              static_cast<unsigned>(length), ESP.getFreeHeap());
            }
            handleFrame(reinterpret_cast<const char*>(payload));
            break;
        case WStype_DISCONNECTED:
            Serial.println("[realtime] ws disconnected");
            handleConnectionFailure();
            break;
        case WStype_ERROR:
            // TLS ハンドシェイク失敗・認証エラーは DISCONNECTED でなく ERROR として通知される
            // ことがあるため、どちらの経路でも画面にエラー表示できるよう同じ処理にする
            Serial.println("[realtime] ws error");
            handleConnectionFailure();
            break;
        default:
            break;
    }
}

}  // namespace

void realtimeConnect(const String& apiKey, const String& instructions, RealtimeCallbacks cb) {
    callbacks = cb;
    if (apiKey.length() == 0) {
        setState(RealtimeState::ErrorState);
        if (callbacks.onError) callbacks.onError("API キーが未登録です");
        return;
    }
    if (!ensureBuffersAllocated()) {
        setState(RealtimeState::ErrorState);
        if (callbacks.onError) callbacks.onError("音声バッファの確保に失敗しました");
        return;
    }
    pendingInstructions = instructions;
    disconnectRequested = false;
    sessionCostJpy = 0.0;
    if (callbacks.onCostUpdated) callbacks.onCostUpdated(sessionCostJpy);
    ws.beginSSL(REALTIME_HOST, REALTIME_PORT, REALTIME_PATH);
    ws.setExtraHeaders(("Authorization: Bearer " + apiKey).c_str());
    ws.onEvent(onWsEvent);
    setState(RealtimeState::Connecting);
}

void realtimeDisconnect() {
    // ws.disconnect() が WStype_DISCONNECTED を同期的に発火させても
    // handleConnectionFailure() がエラー扱いしないよう、切断要求を先に立てる
    disconnectRequested = true;
    ws.disconnect();
    M5.Mic.end();
    M5.Speaker.end();
    setState(RealtimeState::Idle);
    disconnectRequested = false;
}

void realtimeLoop() {
    if (state == RealtimeState::Idle) return;
    ws.loop();

    if (state == RealtimeState::Listening && recordChunkBuf) {
        // record は直前チャンクの取り込み完了を待ってからキューするため連続録音になる
        if (M5.Mic.record(recordChunkBuf, CHUNK_SAMPLES, AUDIO_SAMPLE_RATE)) {
            std::string base64Audio = audio_codec::encodeBase64(
                reinterpret_cast<uint8_t*>(recordChunkBuf), CHUNK_SAMPLES * sizeof(int16_t));
            std::string json = realtime_protocol::buildInputAudioAppendEvent(base64Audio);
            ws.sendTXT(json.c_str());
        }
    } else if (state == RealtimeState::Speaking) {
        if (M5.Speaker.isPlaying(0) == 0) {
            M5.Speaker.end();
            startListening();
        }
    }
}

RealtimeState realtimeCurrentState() {
    return state;
}
