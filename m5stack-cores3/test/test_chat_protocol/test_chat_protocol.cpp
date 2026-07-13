// Responses API(POST /v1/responses)のリクエスト組み立て・レスポンス解釈(issue 00015 受け入れ基準)のテスト。
// 仕様: system instructions は messages に混ぜず instructions フィールドで渡し(会話履歴 + 新規発話のみを
// input 配列に積む)、web_search ツールを宣言する。レスポンスは output[] の message アイテムの
// content[].output_text を連結し、web_search_call アイテムは無視する。usage は input/output/cached tokens。
#include <unity.h>

#include "pure/chat_protocol.h"

using chat_protocol::buildChatRequest;
using chat_protocol::ChatResponse;
using chat_protocol::History;
using chat_protocol::Message;
using chat_protocol::parseChatResponse;

void setUp() {}
void tearDown() {}

static bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

static void test_build_request_has_model_and_instructions_not_in_input() {
    std::string json = buildChatRequest("gpt-5-nano", "あなたは相棒です", {}, "こんにちは");
    TEST_ASSERT_TRUE(contains(json, "\"model\":\"gpt-5-nano\""));
    TEST_ASSERT_TRUE(contains(json, "\"instructions\":\"あなたは相棒です\""));
    // instructions はトップレベルのフィールドであり、input 配列に system ロールとして混ざらない
    TEST_ASSERT_FALSE(contains(json, "\"role\":\"system\""));
    TEST_ASSERT_TRUE(contains(json, "\"role\":\"user\""));
    TEST_ASSERT_TRUE(contains(json, "\"content\":\"こんにちは\""));
}

static void test_build_request_declares_web_search_tool() {
    std::string json = buildChatRequest("gpt-5-nano", "system", {}, "hello");
    TEST_ASSERT_TRUE(contains(json, "\"tools\":[{\"type\":\"web_search\"}]"));
}

static void test_build_request_includes_history_before_new_message_in_input() {
    History history = {{"user", "前回の質問"}, {"assistant", "前回の回答"}};
    std::string json = buildChatRequest("gpt-5-nano", "system", history, "新しい質問");
    size_t historyUserPos = json.find("前回の質問");
    size_t historyAssistantPos = json.find("前回の回答");
    size_t newUserPos = json.find("新しい質問");
    TEST_ASSERT_TRUE(historyUserPos != std::string::npos);
    TEST_ASSERT_TRUE(historyAssistantPos != std::string::npos);
    TEST_ASSERT_TRUE(newUserPos != std::string::npos);
    TEST_ASSERT_TRUE(historyUserPos < historyAssistantPos);
    TEST_ASSERT_TRUE(historyAssistantPos < newUserPos);
}

static void test_build_request_escapes_quotes_in_text() {
    std::string json = buildChatRequest("gpt-5-nano", "system", {}, "\"hello\"");
    TEST_ASSERT_TRUE(contains(json, "\\\"hello\\\""));
}

static void test_parse_response_extracts_output_text_and_usage() {
    ChatResponse resp = parseChatResponse(R"({
        "output": [
            {"type": "message", "role": "assistant", "content": [
                {"type": "output_text", "text": "こんにちは、元気です"}
            ]}
        ],
        "usage": {
            "input_tokens": 120,
            "output_tokens": 40,
            "total_tokens": 160,
            "input_tokens_details": {"cached_tokens": 32}
        }
    })");
    TEST_ASSERT_TRUE(resp.ok);
    TEST_ASSERT_EQUAL_STRING("こんにちは、元気です", resp.content.c_str());
    TEST_ASSERT_EQUAL(120, resp.usage.promptTokens);
    TEST_ASSERT_EQUAL(40, resp.usage.completionTokens);
    TEST_ASSERT_EQUAL(32, resp.usage.cachedTokens);
}

static void test_parse_response_ignores_web_search_call_items() {
    ChatResponse resp = parseChatResponse(R"({
        "output": [
            {"type": "web_search_call", "id": "ws_1", "status": "completed"},
            {"type": "message", "role": "assistant", "content": [
                {"type": "output_text", "text": "東京は晴れです"}
            ]}
        ],
        "usage": {"input_tokens": 200, "output_tokens": 50, "input_tokens_details": {"cached_tokens": 0}}
    })");
    TEST_ASSERT_TRUE(resp.ok);
    TEST_ASSERT_EQUAL_STRING("東京は晴れです", resp.content.c_str());
}

static void test_parse_response_concatenates_multiple_output_text_parts() {
    ChatResponse resp = parseChatResponse(R"({
        "output": [
            {"type": "message", "role": "assistant", "content": [
                {"type": "output_text", "text": "前半"},
                {"type": "output_text", "text": "後半"}
            ]}
        ],
        "usage": {"input_tokens": 10, "output_tokens": 5}
    })");
    TEST_ASSERT_TRUE(resp.ok);
    TEST_ASSERT_EQUAL_STRING("前半後半", resp.content.c_str());
}

static void test_parse_response_without_usage_defaults_to_zero() {
    ChatResponse resp = parseChatResponse(R"({
        "output": [
            {"type": "message", "role": "assistant", "content": [{"type": "output_text", "text": "hi"}]}
        ]
    })");
    TEST_ASSERT_TRUE(resp.ok);
    TEST_ASSERT_EQUAL(0, resp.usage.promptTokens);
    TEST_ASSERT_EQUAL(0, resp.usage.cachedTokens);
}

static void test_parse_malformed_json_is_not_ok() {
    ChatResponse resp = parseChatResponse("{not valid json");
    TEST_ASSERT_FALSE(resp.ok);
    TEST_ASSERT_EQUAL_STRING("", resp.content.c_str());
}

static void test_parse_missing_output_is_not_ok() {
    ChatResponse resp = parseChatResponse(R"({"error":{"message":"invalid api key"}})");
    TEST_ASSERT_FALSE(resp.ok);
}

static void test_parse_output_without_message_item_is_not_ok() {
    ChatResponse resp = parseChatResponse(R"({
        "output": [{"type": "web_search_call", "id": "ws_1", "status": "completed"}]
    })");
    TEST_ASSERT_FALSE(resp.ok);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_build_request_has_model_and_instructions_not_in_input);
    RUN_TEST(test_build_request_declares_web_search_tool);
    RUN_TEST(test_build_request_includes_history_before_new_message_in_input);
    RUN_TEST(test_build_request_escapes_quotes_in_text);
    RUN_TEST(test_parse_response_extracts_output_text_and_usage);
    RUN_TEST(test_parse_response_ignores_web_search_call_items);
    RUN_TEST(test_parse_response_concatenates_multiple_output_text_parts);
    RUN_TEST(test_parse_response_without_usage_defaults_to_zero);
    RUN_TEST(test_parse_malformed_json_is_not_ok);
    RUN_TEST(test_parse_missing_output_is_not_ok);
    RUN_TEST(test_parse_output_without_message_item_is_not_ok);
    return UNITY_END();
}
