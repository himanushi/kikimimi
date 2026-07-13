#include "realtime_protocol.h"

#include <ArduinoJson.h>

namespace realtime_protocol {

std::string buildSessionUpdateEvent(const std::string& instructions) {
    JsonDocument doc;
    doc["type"] = "session.update";
    JsonObject session = doc["session"].to<JsonObject>();
    session["type"] = "realtime";
    session["model"] = "gpt-realtime";
    JsonArray modalities = session["output_modalities"].to<JsonArray>();
    modalities.add("text");
    session["instructions"] = instructions;

    std::string out;
    serializeJson(doc, out);
    return out;
}

std::string buildUserMessageEvent(const std::string& text) {
    JsonDocument doc;
    doc["type"] = "conversation.item.create";
    JsonObject item = doc["item"].to<JsonObject>();
    item["type"] = "message";
    item["role"] = "user";
    JsonArray content = item["content"].to<JsonArray>();
    JsonObject part = content.add<JsonObject>();
    part["type"] = "input_text";
    part["text"] = text;

    std::string out;
    serializeJson(doc, out);
    return out;
}

std::string buildResponseCreateEvent() {
    JsonDocument doc;
    doc["type"] = "response.create";

    std::string out;
    serializeJson(doc, out);
    return out;
}

ServerEvent parseServerEvent(const std::string& json) {
    ServerEvent ev;
    JsonDocument doc;
    if (deserializeJson(doc, json)) return ev;  // 壊れた JSON は Unknown のまま

    const char* type = doc["type"] | "";
    if (std::string(type) == "session.created") {
        ev.type = ServerEventType::SessionCreated;
    } else if (std::string(type) == "session.updated") {
        ev.type = ServerEventType::SessionUpdated;
    } else if (std::string(type) == "response.output_text.delta") {
        ev.type = ServerEventType::ResponseOutputTextDelta;
        ev.textDelta = std::string(doc["delta"] | "");
    } else if (std::string(type) == "response.done") {
        ev.type = ServerEventType::ResponseDone;
    } else if (std::string(type) == "error") {
        ev.type = ServerEventType::Error;
        ev.errorMessage = std::string(doc["error"]["message"] | "");
    }
    return ev;
}

}  // namespace realtime_protocol
