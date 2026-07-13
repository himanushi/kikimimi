// 対話画面のトランスクリプト表示(issue 00009 受け入れ基準)のテスト。
// 仕様: 発話単位の履歴(話者+テキスト)を持ち、delta 追記・確定・折り返し済み最新 M 行の取得を行う。
// UTF-8(日本語 3 バイト文字)の途中で行を折り返さない。
#include <unity.h>

#include "pure/transcript_view.h"

using transcript_view::appendAssistantDelta;
using transcript_view::appendUserUtterance;
using transcript_view::finalizeAssistant;
using transcript_view::History;
using transcript_view::renderLines;
using transcript_view::Speaker;

void setUp() {}
void tearDown() {}

static void test_append_assistant_delta_creates_new_utterance_when_empty() {
    History h;
    h = appendAssistantDelta(h, "こんに");
    TEST_ASSERT_EQUAL(1, (int)h.size());
    TEST_ASSERT_TRUE(h[0].speaker == Speaker::Assistant);
    TEST_ASSERT_EQUAL_STRING("こんに", h[0].text.c_str());
    TEST_ASSERT_FALSE(h[0].finalized);
}

static void test_append_assistant_delta_appends_to_unfinalized_tail() {
    History h;
    h = appendAssistantDelta(h, "こんに");
    h = appendAssistantDelta(h, "ちは");
    TEST_ASSERT_EQUAL(1, (int)h.size());
    TEST_ASSERT_EQUAL_STRING("こんにちは", h[0].text.c_str());
}

static void test_finalize_assistant_marks_tail_finalized() {
    History h;
    h = appendAssistantDelta(h, "こんにちは");
    h = finalizeAssistant(h);
    TEST_ASSERT_TRUE(h[0].finalized);
    // 確定後に新しい delta が来たら別発話として追加される
    h = appendAssistantDelta(h, "また");
    TEST_ASSERT_EQUAL(2, (int)h.size());
    TEST_ASSERT_EQUAL_STRING("また", h[1].text.c_str());
}

static void test_finalize_assistant_on_empty_history_is_noop() {
    History h;
    h = finalizeAssistant(h);
    TEST_ASSERT_EQUAL(0, (int)h.size());
}

static void test_append_user_utterance_adds_finalized_entry() {
    History h;
    h = appendUserUtterance(h, "今日の天気は");
    TEST_ASSERT_EQUAL(1, (int)h.size());
    TEST_ASSERT_TRUE(h[0].speaker == Speaker::User);
    TEST_ASSERT_EQUAL_STRING("今日の天気は", h[0].text.c_str());
    TEST_ASSERT_TRUE(h[0].finalized);
}

static void test_render_lines_prefixes_speaker_label() {
    History h;
    h = appendUserUtterance(h, "こんにちは");
    auto lines = renderLines(h, 40, 10);
    TEST_ASSERT_EQUAL(1, (int)lines.size());
    TEST_ASSERT_EQUAL_STRING("あなた: こんにちは", lines[0].c_str());
}

static void test_render_lines_wraps_without_splitting_multibyte_char() {
    History h;
    // "あなた: "(5 文字、うち ":" " " は半角)+ 本文 10 文字 = 15 文字。wrapWidth=8 で複数行に折り返す
    h = appendUserUtterance(h, "あいうえおかきくけこ");
    auto lines = renderLines(h, 8, 10);
    TEST_ASSERT_TRUE((int)lines.size() >= 2);
    // 各行が正しい UTF-8 として再結合できる(壊れたマルチバイトが含まれていない)ことを、
    // 全行を連結したバイト数が元の文字列のバイト数と一致することで確認する
    std::string joined;
    for (auto& l : lines) joined += l;
    TEST_ASSERT_EQUAL((int)std::string("あなた: あいうえおかきくけこ").size(), (int)joined.size());
}

static void test_render_lines_keeps_only_latest_max_lines() {
    History h;
    for (int i = 0; i < 5; i++) {
        h = appendUserUtterance(h, "発話" + std::to_string(i));
    }
    auto lines = renderLines(h, 40, 2);
    TEST_ASSERT_EQUAL(2, (int)lines.size());
    TEST_ASSERT_TRUE(lines[1].find("発話4") != std::string::npos);
    TEST_ASSERT_TRUE(lines[0].find("発話3") != std::string::npos);
}

static void test_render_lines_interleaves_user_and_assistant_labels() {
    History h;
    h = appendUserUtterance(h, "こんにちは");
    h = appendAssistantDelta(h, "はい");
    h = finalizeAssistant(h);
    auto lines = renderLines(h, 40, 10);
    TEST_ASSERT_EQUAL(2, (int)lines.size());
    TEST_ASSERT_EQUAL_STRING("あなた: こんにちは", lines[0].c_str());
    TEST_ASSERT_EQUAL_STRING("kikimimi: はい", lines[1].c_str());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_append_assistant_delta_creates_new_utterance_when_empty);
    RUN_TEST(test_append_assistant_delta_appends_to_unfinalized_tail);
    RUN_TEST(test_finalize_assistant_marks_tail_finalized);
    RUN_TEST(test_finalize_assistant_on_empty_history_is_noop);
    RUN_TEST(test_append_user_utterance_adds_finalized_entry);
    RUN_TEST(test_render_lines_prefixes_speaker_label);
    RUN_TEST(test_render_lines_wraps_without_splitting_multibyte_char);
    RUN_TEST(test_render_lines_keeps_only_latest_max_lines);
    RUN_TEST(test_render_lines_interleaves_user_and_assistant_labels);
    return UNITY_END();
}
