// OpenAI Realtime API(GA, gpt-realtime)のイベント組み立て・解釈。
// デバイス非依存の純関数(native 環境でテストする)
#pragma once

#include <string>

namespace realtime_protocol {

// session.update: 応答をテキストのみに固定し、instructions を設定する
std::string buildSessionUpdateEvent(const std::string& instructions);

// conversation.item.create: role=user のテキストメッセージを追加する
std::string buildUserMessageEvent(const std::string& text);

// response.create: 直近の会話に対する応答生成を開始する
std::string buildResponseCreateEvent();

enum class ServerEventType {
    SessionCreated,
    SessionUpdated,
    ResponseOutputTextDelta,
    ResponseDone,
    Error,
    Unknown,
};

struct ServerEvent {
    ServerEventType type = ServerEventType::Unknown;
    std::string textDelta;     // ResponseOutputTextDelta のとき、追加されたテキスト断片
    std::string errorMessage;  // Error のとき、表示用メッセージ
};

// サーバイベントの type を判別する。JSON が壊れている・type が未知の場合は Unknown を返す
ServerEvent parseServerEvent(const std::string& json);

}  // namespace realtime_protocol
