// OpenAI Chat Completions API(POST /v1/chat/completions)のリクエスト組み立てとレスポンス解釈。
// デバイス非依存の純関数(native 環境でテストする)
#pragma once
#include <string>
#include <vector>

namespace chat_protocol {

struct Message {
    std::string role;     // "system" | "user" | "assistant"
    std::string content;
};

using History = std::vector<Message>;

// system instructions + これまでの会話履歴 + 新規のユーザー発話から、
// chat/completions のリクエスト JSON({"model":..,"messages":[...]})を組み立てる
std::string buildChatRequest(const std::string& model, const std::string& systemInstructions,
                              const History& history, const std::string& userText);

// prompt_tokens_details.cached_tokens は promptTokens の内数(別枠加算ではない)
struct Usage {
    long promptTokens = 0;
    long completionTokens = 0;
    long cachedTokens = 0;
};

struct ChatResponse {
    bool ok = false;      // choices[0].message.content を取り出せたか
    std::string content;  // アシスタント応答テキスト(ok=false なら空)
    Usage usage;
};

// chat/completions のレスポンス JSON を解釈する。壊れた JSON・content 欠如時は ok=false を返す
ChatResponse parseChatResponse(const std::string& json);

}  // namespace chat_protocol
