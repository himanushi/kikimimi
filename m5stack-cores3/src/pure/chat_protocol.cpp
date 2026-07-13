#include "chat_protocol.h"

#include <ArduinoJson.h>

namespace chat_protocol {

std::string buildChatRequest(const std::string& model, const std::string& systemInstructions,
                              const History& history, const std::string& userText) {
    JsonDocument doc;
    doc["model"] = model;
    doc["instructions"] = systemInstructions;

    JsonArray input = doc["input"].to<JsonArray>();
    for (const auto& m : history) {
        JsonObject entry = input.add<JsonObject>();
        entry["role"] = m.role;
        entry["content"] = m.content;
    }
    JsonObject user = input.add<JsonObject>();
    user["role"] = "user";
    user["content"] = userText;

    JsonArray tools = doc["tools"].to<JsonArray>();
    JsonObject webSearch = tools.add<JsonObject>();
    webSearch["type"] = "web_search";

    std::string out;
    serializeJson(doc, out);
    return out;
}

ChatResponse parseChatResponse(const std::string& json) {
    ChatResponse resp;
    JsonDocument doc;
    if (deserializeJson(doc, json)) return resp;  // 壊れた JSON は ok=false のまま

    JsonArrayConst output = doc["output"];
    std::string content;
    bool foundMessage = false;
    for (JsonObjectConst item : output) {
        const char* type = item["type"] | "";
        if (std::string(type) != "message") continue;  // web_search_call 等は本文を持たないので無視
        foundMessage = true;
        for (JsonObjectConst part : item["content"].as<JsonArrayConst>()) {
            const char* partType = part["type"] | "";
            if (std::string(partType) != "output_text") continue;
            const char* text = part["text"] | "";
            content += text;
        }
    }
    if (!foundMessage) return resp;  // message アイテムが無ければ本文なしとみなす

    resp.ok = true;
    resp.content = content;

    JsonObjectConst usage = doc["usage"];
    resp.usage.promptTokens = usage["input_tokens"] | 0L;
    resp.usage.completionTokens = usage["output_tokens"] | 0L;
    resp.usage.cachedTokens = usage["input_tokens_details"]["cached_tokens"] | 0L;

    return resp;
}

}  // namespace chat_protocol
