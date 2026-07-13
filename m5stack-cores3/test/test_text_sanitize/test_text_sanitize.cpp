// 応答テキストの読み上げ用サニタイズ(URL・出典表記の除去)のテスト。
// 仕様: markdown リンクはラベルだけ残す。生 URL は除去する。URL だけを括った括弧は
// 括弧ごと消す。URL を含まないテキスト(日本語含む)はそのまま。
#include <unity.h>

#include "pure/text_sanitize.h"

using text_sanitize::sanitizeForSpeech;

void setUp() {}
void tearDown() {}

static void test_plain_japanese_text_is_unchanged() {
    std::string in = "今日の東京は晴れ、最高気温は33度の見込みです。";
    TEST_ASSERT_EQUAL_STRING(in.c_str(), sanitizeForSpeech(in).c_str());
}

static void test_raw_url_is_removed() {
    std::string out = sanitizeForSpeech("詳しくは https://example.com/weather/tokyo を見てください。");
    TEST_ASSERT_EQUAL_STRING("詳しくは を見てください。", out.c_str());
}

static void test_markdown_link_keeps_label_only() {
    std::string out = sanitizeForSpeech("出典は [気象庁](https://www.jma.go.jp/) です。");
    TEST_ASSERT_EQUAL_STRING("出典は 気象庁 です。", out.c_str());
}

static void test_parenthesized_url_is_removed_with_parens() {
    std::string out = sanitizeForSpeech("今日は晴れです(https://example.com)。");
    TEST_ASSERT_EQUAL_STRING("今日は晴れです。", out.c_str());
    std::string outAscii = sanitizeForSpeech("Sunny today (http://example.com).");
    TEST_ASSERT_EQUAL_STRING("Sunny today.", outAscii.c_str());
}

static void test_multiple_urls_are_all_removed() {
    std::string out = sanitizeForSpeech("A https://a.example B http://b.example C");
    TEST_ASSERT_EQUAL_STRING("A B C", out.c_str());
}

static void test_url_stops_at_multibyte_character() {
    // URL 直後に句読点や日本語が続いても、日本語部分を巻き込んで消さない
    std::string out = sanitizeForSpeech("https://example.com/today晴れです");
    TEST_ASSERT_EQUAL_STRING("晴れです", out.c_str());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_plain_japanese_text_is_unchanged);
    RUN_TEST(test_raw_url_is_removed);
    RUN_TEST(test_markdown_link_keeps_label_only);
    RUN_TEST(test_parenthesized_url_is_removed_with_parens);
    RUN_TEST(test_multiple_urls_are_all_removed);
    RUN_TEST(test_url_stops_at_multibyte_character);
    return UNITY_END();
}
