// OpenAI Responses API(POST /v1/responses)のリクエスト組み立てとレスポンス解釈。
// デバイス非依存の純関数(native 環境でテストする)
#pragma once
#include <string>
#include <vector>

namespace chat_protocol {

struct Message {
    std::string role;     // "user" | "assistant"(system は instructions フィールドで渡す)
    std::string content;
};

using History = std::vector<Message>;

// system instructions は input に混ぜず instructions フィールドで渡し、これまでの会話履歴 +
// 新規のユーザー発話を input 配列に積んで、web_search ツールを宣言したリクエスト JSON を組み立てる
// ({"model":..,"instructions":..,"input":[...],"tools":[{"type":"web_search"}]})
std::string buildChatRequest(const std::string& model, const std::string& systemInstructions,
                              const History& history, const std::string& userText);

// input_tokens_details.cached_tokens は promptTokens の内数(別枠加算ではない)
struct Usage {
    long promptTokens = 0;
    long completionTokens = 0;
    long cachedTokens = 0;
};

struct ChatResponse {
    bool ok = false;      // output[] の message アイテムから output_text を取り出せたか
    std::string content;  // アシスタント応答テキスト(ok=false なら空)
    Usage usage;
};

// Responses API のレスポンス JSON を解釈する。output[] の type=="message" アイテムの
// content[].output_text を連結する(web_search_call 等の他アイテムは無視する)。
// 壊れた JSON・message アイテム欠如時は ok=false を返す
ChatResponse parseChatResponse(const std::string& json);

}  // namespace chat_protocol
