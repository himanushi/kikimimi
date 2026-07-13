// 設定画面の「WiFi / API キー設定」導線判定(issue 00010 受け入れ基準)のテスト。
// 仕様: WiFi 接続済みなら STA の URL 案内、未接続なら AP ポータルへフォールバックする。
#include <unity.h>

#include "pure/settings_entry.h"

void setUp() {}
void tearDown() {}

static void test_wifi_connected_shows_sta_url_guide() {
    TEST_ASSERT_TRUE(SettingsEntryAction::ShowStaUrlGuide == settingsEntryAction(true));
}

static void test_wifi_not_connected_starts_ap_portal() {
    TEST_ASSERT_TRUE(SettingsEntryAction::StartApPortal == settingsEntryAction(false));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_wifi_connected_shows_sta_url_guide);
    RUN_TEST(test_wifi_not_connected_starts_ap_portal);
    return UNITY_END();
}
