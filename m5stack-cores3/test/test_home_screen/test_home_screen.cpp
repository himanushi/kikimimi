// ホーム画面(issue 00008 受け入れ基準)のレイアウト・タップ判定のテスト。
// 仕様: 中央に大きい対話開始ボタン、隅に小さい設定ボタン。ボタン内タップ → Talk/Settings、外 → None。
#include <unity.h>

#include "pure/home_screen.h"

namespace {
constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 240;
}  // namespace

void setUp() {}
void tearDown() {}

static void test_center_of_talk_button_returns_talk() {
    HomeLayout layout = homeScreenLayout(SCREEN_W, SCREEN_H);
    int cx = layout.talkButton.x + layout.talkButton.w / 2;
    int cy = layout.talkButton.y + layout.talkButton.h / 2;
    TEST_ASSERT_TRUE(HomeTap::Talk == homeScreenHitTest(SCREEN_W, SCREEN_H, cx, cy));
}

static void test_center_of_settings_button_returns_settings() {
    HomeLayout layout = homeScreenLayout(SCREEN_W, SCREEN_H);
    int cx = layout.settingsButton.x + layout.settingsButton.w / 2;
    int cy = layout.settingsButton.y + layout.settingsButton.h / 2;
    TEST_ASSERT_TRUE(HomeTap::Settings == homeScreenHitTest(SCREEN_W, SCREEN_H, cx, cy));
}

static void test_point_outside_both_buttons_returns_none() {
    // 左上隅は「中央の大ボタン」にも「隅の小ボタン」にも入らない想定
    TEST_ASSERT_TRUE(HomeTap::None == homeScreenHitTest(SCREEN_W, SCREEN_H, 2, 2));
}

static void test_talk_button_is_larger_than_settings_button() {
    HomeLayout layout = homeScreenLayout(SCREEN_W, SCREEN_H);
    TEST_ASSERT_TRUE(layout.talkButton.w > layout.settingsButton.w);
    TEST_ASSERT_TRUE(layout.talkButton.h > layout.settingsButton.h);
}

static void test_talk_button_is_horizontally_centered() {
    HomeLayout layout = homeScreenLayout(SCREEN_W, SCREEN_H);
    int center = layout.talkButton.x + layout.talkButton.w / 2;
    TEST_ASSERT_TRUE(center > SCREEN_W / 2 - 5 && center < SCREEN_W / 2 + 5);
}

static void test_settings_button_is_in_a_corner() {
    HomeLayout layout = homeScreenLayout(SCREEN_W, SCREEN_H);
    // どこか一辺は画面端付近(隅に寄っている)ことだけを確認する
    bool nearRightEdge = layout.settingsButton.x + layout.settingsButton.w > SCREEN_W - 30;
    bool nearLeftEdge = layout.settingsButton.x < 30;
    TEST_ASSERT_TRUE(nearRightEdge || nearLeftEdge);
}

// レイアウト計算結果に依存しない固定座標での確認(320x240 の画面中心と右下隅)
static void test_fixed_screen_center_returns_talk() {
    TEST_ASSERT_TRUE(HomeTap::Talk == homeScreenHitTest(SCREEN_W, SCREEN_H, 160, 132));
}

static void test_fixed_bottom_right_corner_returns_settings() {
    TEST_ASSERT_TRUE(HomeTap::Settings == homeScreenHitTest(SCREEN_W, SCREEN_H, 290, 210));
}

static void test_tap_just_outside_talk_button_returns_none() {
    HomeLayout layout = homeScreenLayout(SCREEN_W, SCREEN_H);
    int xOutside = layout.talkButton.x - 5;
    int yOutside = layout.talkButton.y + layout.talkButton.h / 2;
    // 設定ボタンとも重ならない座標であることを前提に、境界外は無反応であることを確認する
    HomeTap tap = homeScreenHitTest(SCREEN_W, SCREEN_H, xOutside, yOutside);
    TEST_ASSERT_TRUE(HomeTap::None == tap);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_center_of_talk_button_returns_talk);
    RUN_TEST(test_center_of_settings_button_returns_settings);
    RUN_TEST(test_point_outside_both_buttons_returns_none);
    RUN_TEST(test_talk_button_is_larger_than_settings_button);
    RUN_TEST(test_talk_button_is_horizontally_centered);
    RUN_TEST(test_settings_button_is_in_a_corner);
    RUN_TEST(test_fixed_screen_center_returns_talk);
    RUN_TEST(test_fixed_bottom_right_corner_returns_settings);
    RUN_TEST(test_tap_just_outside_talk_button_returns_none);
    return UNITY_END();
}
