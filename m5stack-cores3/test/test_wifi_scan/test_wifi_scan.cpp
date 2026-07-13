// WiFi スキャン結果の整形(issue 00006 受け入れ基準)のテスト。
// 仕様: 空 SSID を除外し、同名 SSID は最も強い RSSI のものだけ残し、RSSI 降順で並べる。
#include <unity.h>

#include "pure/wifi_scan.h"

void setUp() {}
void tearDown() {}

static void test_empty_scan_returns_empty() {
    std::vector<WifiScanEntry> out = dedupeSortWifiScan({});
    TEST_ASSERT_EQUAL(0, out.size());
}

static void test_empty_ssid_is_excluded() {
    std::vector<WifiScanEntry> out = dedupeSortWifiScan({{"", -40}, {"home-wifi", -50}});
    TEST_ASSERT_EQUAL(1, out.size());
    TEST_ASSERT_EQUAL_STRING("home-wifi", out[0].ssid.c_str());
}

static void test_sorted_by_rssi_descending() {
    std::vector<WifiScanEntry> out =
        dedupeSortWifiScan({{"weak-ap", -80}, {"strong-ap", -30}, {"mid-ap", -55}});
    TEST_ASSERT_EQUAL(3, out.size());
    TEST_ASSERT_EQUAL_STRING("strong-ap", out[0].ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("mid-ap", out[1].ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("weak-ap", out[2].ssid.c_str());
}

static void test_duplicate_ssid_keeps_strongest_rssi() {
    std::vector<WifiScanEntry> out =
        dedupeSortWifiScan({{"mesh-ap", -80}, {"mesh-ap", -40}, {"mesh-ap", -60}});
    TEST_ASSERT_EQUAL(1, out.size());
    TEST_ASSERT_EQUAL_STRING("mesh-ap", out[0].ssid.c_str());
    TEST_ASSERT_EQUAL(-40, out[0].rssi);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_empty_scan_returns_empty);
    RUN_TEST(test_empty_ssid_is_excluded);
    RUN_TEST(test_sorted_by_rssi_descending);
    RUN_TEST(test_duplicate_ssid_keeps_strongest_rssi);
    return UNITY_END();
}
