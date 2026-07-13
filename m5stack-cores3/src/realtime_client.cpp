#include "realtime_client.h"

#include <WebSocketsClient.h>

#include "pure/realtime_protocol.h"

namespace {

constexpr const char* REALTIME_HOST = "api.openai.com";
constexpr uint16_t REALTIME_PORT = 443;
constexpr const char* REALTIME_PATH = "/v1/realtime?model=gpt-realtime";

WebSocketsClient ws;
RealtimeState state = RealtimeState::Idle;
RealtimeCallbacks callbacks;
String pendingInstructions;
// realtimeDisconnect() による能動的切断を、WStype_DISCONNECTED/ERROR がその場で同期的に
// 発火した場合にも「エラーではない」と区別するためのフラグ
bool disconnectRequested = false;

void setState(RealtimeState next) {
    state = next;
    if (callbacks.onStateChanged) callbacks.onStateChanged(next);
}

// 確立前の切断・エラーをまとめて処理する(WStype_DISCONNECTED/WStype_ERROR 共通)
void handleConnectionFailure() {
    if (disconnectRequested) return;  // 能動的切断はエラー扱いしない
    if (state == RealtimeState::Idle) return;
    bool wasEstablished = state == RealtimeState::Established;
    setState(RealtimeState::ErrorState);
    if (!wasEstablished && callbacks.onError) {
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
            setState(RealtimeState::Established);
            break;
        case realtime_protocol::ServerEventType::ResponseOutputTextDelta:
            if (callbacks.onResponseTextDelta) callbacks.onResponseTextDelta(ev.textDelta.c_str());
            break;
        case realtime_protocol::ServerEventType::ResponseDone:
            Serial.println("[realtime] response.done");
            break;
        case realtime_protocol::ServerEventType::Error:
            Serial.printf("[realtime] error: %s\n", ev.errorMessage.c_str());
            if (callbacks.onError) callbacks.onError(ev.errorMessage.c_str());
            setState(RealtimeState::ErrorState);
            break;
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
    pendingInstructions = instructions;
    disconnectRequested = false;
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
    setState(RealtimeState::Idle);
    disconnectRequested = false;
}

void realtimeLoop() {
    if (state != RealtimeState::Idle) ws.loop();
}

RealtimeState realtimeCurrentState() {
    return state;
}

void realtimeSendUserMessage(const String& text) {
    if (state != RealtimeState::Established) return;
    std::string item = realtime_protocol::buildUserMessageEvent(text.c_str());
    ws.sendTXT(item.c_str());
    std::string create = realtime_protocol::buildResponseCreateEvent();
    ws.sendTXT(create.c_str());
}
