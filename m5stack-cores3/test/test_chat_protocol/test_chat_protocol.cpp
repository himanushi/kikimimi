// chat/completions のリクエスト組み立て・レスポンス解釈(issue 00014 受け入れ基準)のテスト。
// 仕様: リクエストは system + 会話履歴 + 新規発話の順で messages 配列に積む。
// レスポンスは choices[0].message.content と usage(cached tokens は内数)を取り出す。
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

static void test_build_request_has_model_and_system_message_first() {
    std::string json = buildChatRequest("gpt-5-nano", "あなたは相棒です", {}, "こんにちは");
    TEST_ASSERT_TRUE(contains(json, "\"model\":\"gpt-5-nano\""));
    size_t systemPos = json.find("\"role\":\"system\"");
    size_t userPos = json.find("\"role\":\"user\"");
    TEST_ASSERT_TRUE(systemPos != std::string::npos);
    TEST_ASSERT_TRUE(userPos != std::string::npos);
    TEST_ASSERT_TRUE(systemPos < userPos);
    TEST_ASSERT_TRUE(contains(json, "\"content\":\"あなたは相棒です\""));
    TEST_ASSERT_TRUE(contains(json, "\"content\":\"こんにちは\""));
}

static void test_build_request_includes_history_between_system_and_new_message() {
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

static void test_parse_response_extracts_content_and_usage() {
    ChatResponse resp = parseChatResponse(R"({
        "choices": [{"message": {"role": "assistant", "content": "こんにちは、元気です"}}],
        "usage": {
            "prompt_tokens": 120,
            "completion_tokens": 40,
            "total_tokens": 160,
            "prompt_tokens_details": {"cached_tokens": 32}
        }
    })");
    TEST_ASSERT_TRUE(resp.ok);
    TEST_ASSERT_EQUAL_STRING("こんにちは、元気です", resp.content.c_str());
    TEST_ASSERT_EQUAL(120, resp.usage.promptTokens);
    TEST_ASSERT_EQUAL(40, resp.usage.completionTokens);
    TEST_ASSERT_EQUAL(32, resp.usage.cachedTokens);
}

static void test_parse_response_without_usage_defaults_to_zero() {
    ChatResponse resp = parseChatResponse(R"({
        "choices": [{"message": {"role": "assistant", "content": "hi"}}]
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

static void test_parse_missing_choices_is_not_ok() {
    ChatResponse resp = parseChatResponse(R"({"error":{"message":"invalid api key"}})");
    TEST_ASSERT_FALSE(resp.ok);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_build_request_has_model_and_system_message_first);
    RUN_TEST(test_build_request_includes_history_between_system_and_new_message);
    RUN_TEST(test_build_request_escapes_quotes_in_text);
    RUN_TEST(test_parse_response_extracts_content_and_usage);
    RUN_TEST(test_parse_response_without_usage_defaults_to_zero);
    RUN_TEST(test_parse_malformed_json_is_not_ok);
    RUN_TEST(test_parse_missing_choices_is_not_ok);
    return UNITY_END();
}
