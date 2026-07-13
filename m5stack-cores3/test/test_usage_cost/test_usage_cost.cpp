// トークン使用量 → 金額(円)換算(issue 00007 受け入れ基準)のテスト。
// 仕様: gpt-realtime-mini の公式単価(1M トークンあたり USD, cached input は text/audio で別単価)を
// 160 円/USD の固定レートで円換算する。cached トークンは input トークン数の内数として扱う。
#include <unity.h>

#include "pure/usage_cost.h"

using realtime_protocol::UsageTokens;
using usage_cost::calculateCostJpy;
using usage_cost::formatJpyLabel;

void setUp() {}
void tearDown() {}

static void test_zero_usage_costs_nothing() {
    UsageTokens usage;
    TEST_ASSERT_EQUAL_FLOAT(0.0f, calculateCostJpy(usage));
}

// audio_tokens=1,000,000, cached=0 のとき: $10 * 160 = ¥1600
static void test_audio_input_million_tokens_costs_expected_yen() {
    UsageTokens usage;
    usage.inputAudioTokens = 1000000;
    TEST_ASSERT_EQUAL_FLOAT(1600.0f, calculateCostJpy(usage));
}

// audio_tokens=1,000,000 output のとき: $20 * 160 = ¥3200
static void test_audio_output_million_tokens_costs_expected_yen() {
    UsageTokens usage;
    usage.outputAudioTokens = 1000000;
    TEST_ASSERT_EQUAL_FLOAT(3200.0f, calculateCostJpy(usage));
}

// text_tokens=1,000,000 input のとき: $0.60 * 160 = ¥96
static void test_text_input_million_tokens_costs_expected_yen() {
    UsageTokens usage;
    usage.inputTextTokens = 1000000;
    TEST_ASSERT_EQUAL_FLOAT(96.0f, calculateCostJpy(usage));
}

// text_tokens=1,000,000 output のとき: $2.40 * 160 = ¥384
static void test_text_output_million_tokens_costs_expected_yen() {
    UsageTokens usage;
    usage.outputTextTokens = 1000000;
    TEST_ASSERT_EQUAL_FLOAT(384.0f, calculateCostJpy(usage));
}

// cached はその分だけ非 cached レートでなく cached 音声レート($0.30)で計算される。
// input_audio=1,000,000 のうち cached=1,000,000(全量 cached)なら $0.30 * 160 = ¥48
static void test_fully_cached_audio_input_uses_cached_rate() {
    UsageTokens usage;
    usage.inputAudioTokens = 1000000;
    usage.cachedAudioTokens = 1000000;
    TEST_ASSERT_EQUAL_FLOAT(48.0f, calculateCostJpy(usage));
}

// 一部だけ cached: input_text=1,000,000 のうち cached=400,000
// 非 cached 600,000 * $0.60/M + cached 400,000 * $0.06/M = $0.36 + $0.024 = $0.384 → ¥61.44
static void test_partially_cached_text_input_splits_rates() {
    UsageTokens usage;
    usage.inputTextTokens = 1000000;
    usage.cachedTextTokens = 400000;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 61.44f, calculateCostJpy(usage));
}

// 複数種別の合算(各 100,000 トークン)
// text in $0.06→¥9.6 + audio in $1.00→¥160 + text out $0.24→¥38.4 + audio out $2.00→¥320 = ¥528
static void test_combined_usage_sums_all_categories() {
    UsageTokens usage;
    usage.inputTextTokens = 100000;
    usage.inputAudioTokens = 100000;
    usage.outputTextTokens = 100000;
    usage.outputAudioTokens = 100000;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 528.0f, calculateCostJpy(usage));
}

static void test_format_jpy_label_has_yen_sign_and_one_decimal() {
    TEST_ASSERT_EQUAL_STRING("¥12.3", formatJpyLabel(12.3).c_str());
}

static void test_format_jpy_label_zero() {
    TEST_ASSERT_EQUAL_STRING("¥0.0", formatJpyLabel(0.0).c_str());
}

static void test_format_jpy_label_rounds_to_one_decimal() {
    TEST_ASSERT_EQUAL_STRING("¥12.4", formatJpyLabel(12.36).c_str());
}

// gpt-5-nano: in $0.05/M, out $0.40/M(issue 00014)
static void test_chat_cost_prompt_and_completion_tokens() {
    // prompt 1,000,000 * $0.05 = $0.05 → ¥8, completion 1,000,000 * $0.40 = $0.40 → ¥64
    double jpy = usage_cost::calculateChatCostJpy(1000000, 1000000, 0);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 72.0f, jpy);
}

static void test_chat_cost_zero_usage_costs_nothing() {
    TEST_ASSERT_EQUAL_FLOAT(0.0f, usage_cost::calculateChatCostJpy(0, 0, 0));
}

// cached はキャッシュなし単価と同じ($0.05/M)ため、素朴な合算と一致する
// (issue 00014 では chat の cached 単価をキャッシュなしと区別しない。区別が必要になったら拡張する)
static void test_chat_cost_cached_tokens_are_subset_of_prompt() {
    double withCache = usage_cost::calculateChatCostJpy(1000000, 0, 400000);
    double withoutCache = usage_cost::calculateChatCostJpy(1000000, 0, 0);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, withoutCache, withCache);
}

// gpt-4o-mini-transcribe: audio in $3.00/M, text out $5.00/M(issue 00014)
static void test_stt_cost_audio_input_and_text_output() {
    // audio 1,000,000 * $3.00 = $3.00 → ¥480, text 1,000,000 * $5.00 = $5.00 → ¥800
    double jpy = usage_cost::calculateSttCostJpy(1000000, 1000000);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1280.0f, jpy);
}

static void test_stt_cost_zero_usage_costs_nothing() {
    TEST_ASSERT_EQUAL_FLOAT(0.0f, usage_cost::calculateSttCostJpy(0, 0));
}

// gpt-4o-mini-tts: usage が返らないため入力バイト数・出力 PCM 長からの概算(issue 00014)
static void test_tts_cost_estimate_is_positive_for_nonempty_input_and_output() {
    // 30 文字相当(UTF-8 で概ね 90 バイト)のテキスト、24kHz 16bit mono で 2 秒分の PCM
    size_t pcmBytes = 24000 * 2 /*bytes/sample*/ * 2 /*seconds*/;
    double jpy = usage_cost::estimateTtsCostJpy(90, pcmBytes, 24000);
    TEST_ASSERT_TRUE(jpy > 0.0);
}

static void test_tts_cost_estimate_zero_for_empty_input_and_output() {
    TEST_ASSERT_EQUAL_FLOAT(0.0f, usage_cost::estimateTtsCostJpy(0, 0, 24000));
}

// PCM が長いほど(=再生時間が長いほど)概算コストも増える
static void test_tts_cost_estimate_scales_with_pcm_length() {
    double shortJpy = usage_cost::estimateTtsCostJpy(90, 24000 * 2 * 1, 24000);
    double longJpy = usage_cost::estimateTtsCostJpy(90, 24000 * 2 * 5, 24000);
    TEST_ASSERT_TRUE(longJpy > shortJpy);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_zero_usage_costs_nothing);
    RUN_TEST(test_audio_input_million_tokens_costs_expected_yen);
    RUN_TEST(test_audio_output_million_tokens_costs_expected_yen);
    RUN_TEST(test_text_input_million_tokens_costs_expected_yen);
    RUN_TEST(test_text_output_million_tokens_costs_expected_yen);
    RUN_TEST(test_fully_cached_audio_input_uses_cached_rate);
    RUN_TEST(test_partially_cached_text_input_splits_rates);
    RUN_TEST(test_combined_usage_sums_all_categories);
    RUN_TEST(test_format_jpy_label_has_yen_sign_and_one_decimal);
    RUN_TEST(test_format_jpy_label_zero);
    RUN_TEST(test_format_jpy_label_rounds_to_one_decimal);
    RUN_TEST(test_chat_cost_prompt_and_completion_tokens);
    RUN_TEST(test_chat_cost_zero_usage_costs_nothing);
    RUN_TEST(test_chat_cost_cached_tokens_are_subset_of_prompt);
    RUN_TEST(test_stt_cost_audio_input_and_text_output);
    RUN_TEST(test_stt_cost_zero_usage_costs_nothing);
    RUN_TEST(test_tts_cost_estimate_is_positive_for_nonempty_input_and_output);
    RUN_TEST(test_tts_cost_estimate_zero_for_empty_input_and_output);
    RUN_TEST(test_tts_cost_estimate_scales_with_pcm_length);
    return UNITY_END();
}
