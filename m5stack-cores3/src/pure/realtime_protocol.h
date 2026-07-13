// OpenAI Realtime API(GA, gpt-realtime)のイベント組み立て・解釈。
// デバイス非依存の純関数(native 環境でテストする)
#pragma once

#include <string>

namespace realtime_protocol {

// session.update: 音声応答(PCM16 24kHz, semantic_vad)を設定し、instructions を設定する
std::string buildSessionUpdateEvent(const std::string& instructions);

// conversation.item.create: role=user のテキストメッセージを追加する
std::string buildUserMessageEvent(const std::string& text);

// response.create: 直近の会話に対する応答生成を開始する
std::string buildResponseCreateEvent();

// input_audio_buffer.append: base64 化済みの PCM16 チャンクをそのまま包む
std::string buildInputAudioAppendEvent(const std::string& base64Audio);

enum class ServerEventType {
    SessionCreated,
    SessionUpdated,
    ResponseOutputTextDelta,
    ResponseOutputAudioDelta,
    SpeechStarted,
    SpeechStopped,
    ResponseDone,
    Error,
    Unknown,
};

// response.done の response.usage から取り出したトークン内訳。
// cached系は input の text/audio トークン数の内数(別枠加算ではない)
struct UsageTokens {
    long inputTextTokens = 0;
    long inputAudioTokens = 0;
    long cachedTextTokens = 0;
    long cachedAudioTokens = 0;
    long outputTextTokens = 0;
    long outputAudioTokens = 0;
};

struct ServerEvent {
    ServerEventType type = ServerEventType::Unknown;
    std::string textDelta;     // ResponseOutputTextDelta のとき、追加されたテキスト断片
    std::string audioDelta;    // ResponseOutputAudioDelta のとき、base64 化された PCM16 断片
    std::string errorMessage;  // Error のとき、表示用メッセージ
    UsageTokens usage;         // ResponseDone のとき、response.usage のトークン内訳
};

// サーバイベントの type を判別する。JSON が壊れている・type が未知の場合は Unknown を返す
ServerEvent parseServerEvent(const std::string& json);

}  // namespace realtime_protocol
