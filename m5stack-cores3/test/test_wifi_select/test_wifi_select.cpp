// 保存済み WiFi リストとスキャン結果からの接続候補選択(issue 00011 受け入れ基準)のテスト。
#include <unity.h>

#include "pure/wifi_select.h"

void setUp() {}
void tearDown() {}

static void test_multiple_matches_sorted_by_rssi_descending() {
    std::vector<WifiCredential> saved = {{"home", "p1"}, {"office", "p2"}};
    std::vector<WifiScanEntry> scan = {{"office", -70}, {"home", -40}, {"other", -20}};
    std::vector<WifiCandidate> out = selectWifiCandidates(saved, scan);
    TEST_ASSERT_EQUAL(2, out.size());
    TEST_ASSERT_EQUAL_STRING("home", out[0].credential.ssid.c_str());
    TEST_ASSERT_EQUAL(-40, out[0].rssi);
    TEST_ASSERT_EQUAL_STRING("office", out[1].credential.ssid.c_str());
    TEST_ASSERT_EQUAL(-70, out[1].rssi);
}

static void test_no_match_returns_empty() {
    std::vector<WifiCredential> saved = {{"home", "p1"}};
    std::vector<WifiScanEntry> scan = {{"other-ap", -50}};
    std::vector<WifiCandidate> out = selectWifiCandidates(saved, scan);
    TEST_ASSERT_EQUAL(0, out.size());
}

static void test_same_ssid_multiple_aps_picks_strongest() {
    std::vector<WifiCredential> saved = {{"mesh-home", "p1"}};
    std::vector<WifiScanEntry> scan = {{"mesh-home", -80}, {"mesh-home", -35}, {"mesh-home", -60}};
    std::vector<WifiCandidate> out = selectWifiCandidates(saved, scan);
    TEST_ASSERT_EQUAL(1, out.size());
    TEST_ASSERT_EQUAL(-35, out[0].rssi);
}

static void test_empty_saved_returns_empty() {
    std::vector<WifiCandidate> out = selectWifiCandidates({}, {{"home", -40}});
    TEST_ASSERT_EQUAL(0, out.size());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_multiple_matches_sorted_by_rssi_descending);
    RUN_TEST(test_no_match_returns_empty);
    RUN_TEST(test_same_ssid_multiple_aps_picks_strongest);
    RUN_TEST(test_empty_saved_returns_empty);
    return UNITY_END();
}
