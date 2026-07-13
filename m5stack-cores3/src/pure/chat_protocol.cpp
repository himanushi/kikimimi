#include "chat_protocol.h"

#include <ArduinoJson.h>

namespace chat_protocol {

std::string buildChatRequest(const std::string& model, const std::string& systemInstructions,
                              const History& history, const std::string& userText) {
    JsonDocument doc;
    doc["model"] = model;
    JsonArray messages = doc["messages"].to<JsonArray>();

    JsonObject system = messages.add<JsonObject>();
    system["role"] = "system";
    system["content"] = systemInstructions;

    for (const auto& m : history) {
        JsonObject entry = messages.add<JsonObject>();
        entry["role"] = m.role;
        entry["content"] = m.content;
    }

    JsonObject user = messages.add<JsonObject>();
    user["role"] = "user";
    user["content"] = userText;

    std::string out;
    serializeJson(doc, out);
    return out;
}

ChatResponse parseChatResponse(const std::string& json) {
    ChatResponse resp;
    JsonDocument doc;
    if (deserializeJson(doc, json)) return resp;  // 壊れた JSON は ok=false のまま

    JsonVariantConst content = doc["choices"][0]["message"]["content"];
    if (content.isNull() || !content.is<const char*>()) return resp;

    resp.ok = true;
    resp.content = std::string(content.as<const char*>());

    JsonObjectConst usage = doc["usage"];
    resp.usage.promptTokens = usage["prompt_tokens"] | 0L;
    resp.usage.completionTokens = usage["completion_tokens"] | 0L;
    resp.usage.cachedTokens = usage["prompt_tokens_details"]["cached_tokens"] | 0L;

    return resp;
}

}  // namespace chat_protocol
