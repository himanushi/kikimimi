// Realtime API イベントの組み立て・解釈(issue 00002/00003 受け入れ基準)のテスト。
// 仕様: session.update は音声応答(PCM16 24kHz, semantic_vad)を設定し instructions を設定する。
// ユーザーメッセージは conversation.item.create → response.create の順で送る。
// マイク音声は input_audio_buffer.append で base64 チャンクを送る。
// サーバイベントは type で判別し、response.output_text.delta は delta を、
// response.output_audio.delta は base64 の delta を、error は error.message を取り出す。
#include <unity.h>

#include "pure/realtime_protocol.h"

using realtime_protocol::buildInputAudioAppendEvent;
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

static void test_session_update_event_configures_audio_io_with_instructions() {
    std::string json = buildSessionUpdateEvent("あなたは相棒です");
    TEST_ASSERT_TRUE(contains(json, "\"type\":\"session.update\""));
    TEST_ASSERT_TRUE(contains(json, "\"type\":\"realtime\""));
    TEST_ASSERT_TRUE(contains(json, "\"model\":\"gpt-realtime\""));
    TEST_ASSERT_TRUE(contains(json, "\"output_modalities\":[\"audio\"]"));
    TEST_ASSERT_TRUE(contains(json, "\"instructions\":\"あなたは相棒です\""));
    // 入出力とも PCM16 24kHz。rate は入力・出力の両方で必須
    // (出力側が欠けると API が "missing required parameter: session.audio.output.format.rate" を返す)
    size_t first = json.find("\"format\":{\"type\":\"audio/pcm\",\"rate\":24000}");
    TEST_ASSERT_TRUE(first != std::string::npos);
    TEST_ASSERT_TRUE(json.find("\"format\":{\"type\":\"audio/pcm\",\"rate\":24000}", first + 1) !=
                     std::string::npos);
    // semantic_vad(サーバ側 VAD に任せ、push-to-talk にしない)
    TEST_ASSERT_TRUE(contains(json, "\"turn_detection\":{\"type\":\"semantic_vad\"}"));
}

static void test_input_audio_append_event_wraps_base64_audio() {
    std::string json = buildInputAudioAppendEvent("QUJD");
    TEST_ASSERT_TRUE(contains(json, "\"type\":\"input_audio_buffer.append\""));
    TEST_ASSERT_TRUE(contains(json, "\"audio\":\"QUJD\""));
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

static void test_parse_response_output_audio_delta_extracts_base64() {
    auto ev = parseServerEvent(
        R"({"type":"response.output_audio.delta","item_id":"msg_1","delta":"QUJD"})");
    TEST_ASSERT_TRUE(ev.type == ServerEventType::ResponseOutputAudioDelta);
    TEST_ASSERT_EQUAL_STRING("QUJD", ev.audioDelta.c_str());
}

static void test_parse_speech_started() {
    auto ev = parseServerEvent(R"({"type":"input_audio_buffer.speech_started","item_id":"msg_1"})");
    TEST_ASSERT_TRUE(ev.type == ServerEventType::SpeechStarted);
}

static void test_parse_speech_stopped() {
    auto ev = parseServerEvent(R"({"type":"input_audio_buffer.speech_stopped","item_id":"msg_1"})");
    TEST_ASSERT_TRUE(ev.type == ServerEventType::SpeechStopped);
}

static void test_parse_response_done() {
    auto ev = parseServerEvent(R"({"type":"response.done","response":{"id":"resp_1"}})");
    TEST_ASSERT_TRUE(ev.type == ServerEventType::ResponseDone);
}

// response.usage の input_token_details / output_token_details からトークン内訳を取り出す。
// cached_tokens_details は input の text/audio トークン数の内数(00007 plan)
static void test_parse_response_done_extracts_usage_token_breakdown() {
    auto ev = parseServerEvent(R"({
        "type":"response.done",
        "response":{
            "id":"resp_1",
            "usage":{
                "total_tokens":176,
                "input_tokens":89,
                "output_tokens":87,
                "input_token_details":{
                    "text_tokens":31,
                    "audio_tokens":58,
                    "cached_tokens":20,
                    "cached_tokens_details":{"text_tokens":10,"audio_tokens":10}
                },
                "output_token_details":{
                    "text_tokens":30,
                    "audio_tokens":57
                }
            }
        }
    })");
    TEST_ASSERT_TRUE(ev.type == ServerEventType::ResponseDone);
    TEST_ASSERT_EQUAL(31, ev.usage.inputTextTokens);
    TEST_ASSERT_EQUAL(58, ev.usage.inputAudioTokens);
    TEST_ASSERT_EQUAL(10, ev.usage.cachedTextTokens);
    TEST_ASSERT_EQUAL(10, ev.usage.cachedAudioTokens);
    TEST_ASSERT_EQUAL(30, ev.usage.outputTextTokens);
    TEST_ASSERT_EQUAL(57, ev.usage.outputAudioTokens);
}

// usage が欠けたフレーム(session.update 直後など想定しないが)は 0 埋めのまま
static void test_parse_response_done_without_usage_defaults_to_zero() {
    auto ev = parseServerEvent(R"({"type":"response.done","response":{"id":"resp_1"}})");
    TEST_ASSERT_EQUAL(0, ev.usage.inputTextTokens);
    TEST_ASSERT_EQUAL(0, ev.usage.outputAudioTokens);
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
    RUN_TEST(test_session_update_event_configures_audio_io_with_instructions);
    RUN_TEST(test_input_audio_append_event_wraps_base64_audio);
    RUN_TEST(test_user_message_event_wraps_text_as_input_text_content);
    RUN_TEST(test_user_message_event_escapes_quotes_in_text);
    RUN_TEST(test_response_create_event_has_response_create_type);
    RUN_TEST(test_parse_session_created);
    RUN_TEST(test_parse_session_updated);
    RUN_TEST(test_parse_response_output_text_delta_extracts_delta);
    RUN_TEST(test_parse_response_output_audio_delta_extracts_base64);
    RUN_TEST(test_parse_speech_started);
    RUN_TEST(test_parse_speech_stopped);
    RUN_TEST(test_parse_response_done);
    RUN_TEST(test_parse_response_done_extracts_usage_token_breakdown);
    RUN_TEST(test_parse_response_done_without_usage_defaults_to_zero);
    RUN_TEST(test_parse_error_extracts_message);
    RUN_TEST(test_parse_unknown_type_is_unknown);
    RUN_TEST(test_parse_malformed_json_is_unknown_not_crash);
    return UNITY_END();
}
