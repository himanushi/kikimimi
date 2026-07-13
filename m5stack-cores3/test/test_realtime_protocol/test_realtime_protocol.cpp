// Realtime API イベントの組み立て・解釈(issue 00002 受け入れ基準)のテスト。
// 仕様: session.update はテキスト応答のみに固定し instructions を設定する。
// ユーザーメッセージは conversation.item.create → response.create の順で送る。
// サーバイベントは type で判別し、response.output_text.delta は delta を、
// error は error.message を取り出す。
#include <unity.h>

#include "pure/realtime_protocol.h"

using realtime_protocol::buildResponseCreateEvent;
using realtime_protocol::buildSessionUpdateEvent;
using realtime_protocol::buildUserMessageEvent;
using realtime_protocol::parseServerEvent;
using realtime_protocol::ServerEventType;

void setUp() {}
void tearDown() {}

// JSON の厳密なキー順序に依存せず、値の有無だけを雑に確認する(テストの実質を保つため
// 部分文字列一致にとどめ、パース結果はサーバイベント側のテストで確認する)
static bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

static void test_session_update_event_fixes_text_only_output_with_instructions() {
    std::string json = buildSessionUpdateEvent("あなたは相棒です");
    TEST_ASSERT_TRUE(contains(json, "\"type\":\"session.update\""));
    TEST_ASSERT_TRUE(contains(json, "\"type\":\"realtime\""));
    TEST_ASSERT_TRUE(contains(json, "\"model\":\"gpt-realtime\""));
    TEST_ASSERT_TRUE(contains(json, "\"output_modalities\":[\"text\"]"));
    TEST_ASSERT_TRUE(contains(json, "\"instructions\":\"あなたは相棒です\""));
}

static void test_user_message_event_wraps_text_as_input_text_content() {
    std::string json = buildUserMessageEvent("こんにちは");
    TEST_ASSERT_TRUE(contains(json, "\"type\":\"conversation.item.create\""));
    TEST_ASSERT_TRUE(contains(json, "\"type\":\"message\""));
    TEST_ASSERT_TRUE(contains(json, "\"role\":\"user\""));
    TEST_ASSERT_TRUE(contains(json, "\"type\":\"input_text\""));
    TEST_ASSERT_TRUE(contains(json, "\"text\":\"こんにちは\""));
}

static void test_user_message_event_escapes_quotes_in_text() {
    std::string json = buildUserMessageEvent("\"hello\"");
    TEST_ASSERT_TRUE(contains(json, "\\\"hello\\\""));
}

static void test_response_create_event_has_response_create_type() {
    std::string json = buildResponseCreateEvent();
    TEST_ASSERT_TRUE(contains(json, "\"type\":\"response.create\""));
}

static void test_parse_session_created() {
    auto ev = parseServerEvent(R"({"type":"session.created","session":{"id":"sess_1"}})");
    TEST_ASSERT_TRUE(ev.type == ServerEventType::SessionCreated);
}

static void test_parse_session_updated() {
    auto ev = parseServerEvent(R"({"type":"session.updated","session":{"id":"sess_1"}})");
    TEST_ASSERT_TRUE(ev.type == ServerEventType::SessionUpdated);
}

static void test_parse_response_output_text_delta_extracts_delta() {
    auto ev = parseServerEvent(
        R"({"type":"response.output_text.delta","item_id":"msg_1","delta":"Hi there"})");
    TEST_ASSERT_TRUE(ev.type == ServerEventType::ResponseOutputTextDelta);
    TEST_ASSERT_EQUAL_STRING("Hi there", ev.textDelta.c_str());
}

static void test_parse_response_done() {
    auto ev = parseServerEvent(R"({"type":"response.done","response":{"id":"resp_1"}})");
    TEST_ASSERT_TRUE(ev.type == ServerEventType::ResponseDone);
}

static void test_parse_error_extracts_message() {
    auto ev = parseServerEvent(
        R"({"type":"error","error":{"type":"invalid_request_error","message":"invalid api key"}})");
    TEST_ASSERT_TRUE(ev.type == ServerEventType::Error);
    TEST_ASSERT_EQUAL_STRING("invalid api key", ev.errorMessage.c_str());
}

static void test_parse_unknown_type_is_unknown() {
    auto ev = parseServerEvent(R"({"type":"conversation.item.created","item":{}})");
    TEST_ASSERT_TRUE(ev.type == ServerEventType::Unknown);
}

static void test_parse_malformed_json_is_unknown_not_crash() {
    auto ev = parseServerEvent("{not valid json");
    TEST_ASSERT_TRUE(ev.type == ServerEventType::Unknown);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_session_update_event_fixes_text_only_output_with_instructions);
    RUN_TEST(test_user_message_event_wraps_text_as_input_text_content);
    RUN_TEST(test_user_message_event_escapes_quotes_in_text);
    RUN_TEST(test_response_create_event_has_response_create_type);
    RUN_TEST(test_parse_session_created);
    RUN_TEST(test_parse_session_updated);
    RUN_TEST(test_parse_response_output_text_delta_extracts_delta);
    RUN_TEST(test_parse_response_done);
    RUN_TEST(test_parse_error_extracts_message);
    RUN_TEST(test_parse_unknown_type_is_unknown);
    RUN_TEST(test_parse_malformed_json_is_unknown_not_crash);
    return UNITY_END();
}
