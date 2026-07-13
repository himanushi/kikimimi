// セットアップポータルの表示画面判定(issue 00005 受け入れ基準)のテスト。
// 仕様: AP への接続台数が 0 台なら AP 接続案内、1 台以上なら設定 URL 案内を表示する。
#include <unity.h>

#include "pure/portal_screen.h"

void setUp() {}
void tearDown() {}

static void test_zero_stations_shows_ap_join_screen() {
    TEST_ASSERT_TRUE(PortalScreen::ApJoin == portalScreenForStationCount(0));
}

static void test_one_station_shows_url_guide_screen() {
    TEST_ASSERT_TRUE(PortalScreen::UrlGuide == portalScreenForStationCount(1));
}

static void test_multiple_stations_shows_url_guide_screen() {
    TEST_ASSERT_TRUE(PortalScreen::UrlGuide == portalScreenForStationCount(3));
}

static void test_station_count_returning_to_zero_shows_ap_join_screen_again() {
    TEST_ASSERT_TRUE(PortalScreen::UrlGuide == portalScreenForStationCount(1));
    TEST_ASSERT_TRUE(PortalScreen::ApJoin == portalScreenForStationCount(0));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_zero_stations_shows_ap_join_screen);
    RUN_TEST(test_one_station_shows_url_guide_screen);
    RUN_TEST(test_multiple_stations_shows_url_guide_screen);
    RUN_TEST(test_station_count_returning_to_zero_shows_ap_join_screen_again);
    return UNITY_END();
}
