// 設定画面(issue 00012 受け入れ基準)のレイアウト・タップ判定・音量増減のテスト。
// 仕様: 音量 +/- ボタン、WiFi 設定ボタン、戻るボタン。タップ外は None。
// 音量は 0..255 の範囲でクランプされる。
#include <unity.h>

#include "pure/settings_screen.h"

namespace {
constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 240;
}  // namespace

void setUp() {}
void tearDown() {}

static void test_center_of_volume_up_button_returns_volume_up() {
    SettingsLayout layout = settingsScreenLayout(SCREEN_W, SCREEN_H);
    int cx = layout.volumeUpButton.x + layout.volumeUpButton.w / 2;
    int cy = layout.volumeUpButton.y + layout.volumeUpButton.h / 2;
    TEST_ASSERT_TRUE(SettingsTap::VolumeUp == settingsScreenHitTest(SCREEN_W, SCREEN_H, cx, cy));
}

static void test_center_of_volume_down_button_returns_volume_down() {
    SettingsLayout layout = settingsScreenLayout(SCREEN_W, SCREEN_H);
    int cx = layout.volumeDownButton.x + layout.volumeDownButton.w / 2;
    int cy = layout.volumeDownButton.y + layout.volumeDownButton.h / 2;
    TEST_ASSERT_TRUE(SettingsTap::VolumeDown == settingsScreenHitTest(SCREEN_W, SCREEN_H, cx, cy));
}

static void test_center_of_wifi_setup_button_returns_wifi_setup() {
    SettingsLayout layout = settingsScreenLayout(SCREEN_W, SCREEN_H);
    int cx = layout.wifiSetupButton.x + layout.wifiSetupButton.w / 2;
    int cy = layout.wifiSetupButton.y + layout.wifiSetupButton.h / 2;
    TEST_ASSERT_TRUE(SettingsTap::WifiSetup == settingsScreenHitTest(SCREEN_W, SCREEN_H, cx, cy));
}

static void test_center_of_back_button_returns_back() {
    SettingsLayout layout = settingsScreenLayout(SCREEN_W, SCREEN_H);
    int cx = layout.backButton.x + layout.backButton.w / 2;
    int cy = layout.backButton.y + layout.backButton.h / 2;
    TEST_ASSERT_TRUE(SettingsTap::Back == settingsScreenHitTest(SCREEN_W, SCREEN_H, cx, cy));
}

static void test_point_outside_all_buttons_returns_none() {
    // 各ボタンの間の余白(画面中央やや上)はどのボタンにも入らない想定
    SettingsLayout layout = settingsScreenLayout(SCREEN_W, SCREEN_H);
    int y = layout.volumeUpButton.y - 20;
    TEST_ASSERT_TRUE(y > 0);
    TEST_ASSERT_TRUE(SettingsTap::None == settingsScreenHitTest(SCREEN_W, SCREEN_H, SCREEN_W / 2, y));
}

// 「もどる」は画面最下段の左側に置く(knowhow: m5stack-ui-back-button-rule)
static void test_back_button_is_bottom_left() {
    SettingsLayout layout = settingsScreenLayout(SCREEN_W, SCREEN_H);
    TEST_ASSERT_TRUE(layout.backButton.x < SCREEN_W / 2);
    TEST_ASSERT_TRUE(layout.backButton.y + layout.backButton.h >= SCREEN_H - 5);
}

static void test_volume_up_increases_within_range() {
    TEST_ASSERT_EQUAL_INT(132, settingsVolumeIncrease(100));
}

static void test_volume_up_clamps_at_max() {
    TEST_ASSERT_EQUAL_INT(255, settingsVolumeIncrease(240));
    TEST_ASSERT_EQUAL_INT(255, settingsVolumeIncrease(255));
}

static void test_volume_down_decreases_within_range() {
    TEST_ASSERT_EQUAL_INT(68, settingsVolumeDecrease(100));
}

static void test_volume_down_clamps_at_min() {
    TEST_ASSERT_EQUAL_INT(0, settingsVolumeDecrease(20));
    TEST_ASSERT_EQUAL_INT(0, settingsVolumeDecrease(0));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_center_of_volume_up_button_returns_volume_up);
    RUN_TEST(test_center_of_volume_down_button_returns_volume_down);
    RUN_TEST(test_center_of_wifi_setup_button_returns_wifi_setup);
    RUN_TEST(test_center_of_back_button_returns_back);
    RUN_TEST(test_point_outside_all_buttons_returns_none);
    RUN_TEST(test_back_button_is_bottom_left);
    RUN_TEST(test_volume_up_increases_within_range);
    RUN_TEST(test_volume_up_clamps_at_max);
    RUN_TEST(test_volume_down_decreases_within_range);
    RUN_TEST(test_volume_down_clamps_at_min);
    return UNITY_END();
}
