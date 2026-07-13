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
    return UNITY_END();
}
