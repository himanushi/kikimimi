// WiFi 接続 QR ペイロード生成(issue 00004 受け入れ基準)のテスト。
// 仕様: `WIFI:T:WPA;S:<ssid>;P:<pass>;;` 形式。ssid/pass 中の `\ ; , " :` はバックスラッシュエスケープする。
// パスワードが空(オープンネットワーク)のときは T:nopass; とし P: フィールドは省く。
#include <unity.h>

#include "pure/wifi_qr.h"

void setUp() {}
void tearDown() {}

static void test_basic_payload_has_wpa_ssid_and_password() {
    std::string payload = buildWifiQrPayload("home-wifi", "12345678");
    TEST_ASSERT_EQUAL_STRING("WIFI:T:WPA;S:home-wifi;P:12345678;;", payload.c_str());
}

static void test_open_network_uses_nopass_and_omits_password_field() {
    std::string payload = buildWifiQrPayload("open-wifi", "");
    TEST_ASSERT_EQUAL_STRING("WIFI:T:nopass;S:open-wifi;;", payload.c_str());
}

static void test_semicolon_in_ssid_is_escaped() {
    std::string payload = buildWifiQrPayload("a;b", "pass1234");
    TEST_ASSERT_EQUAL_STRING("WIFI:T:WPA;S:a\\;b;P:pass1234;;", payload.c_str());
}

static void test_comma_in_password_is_escaped() {
    std::string payload = buildWifiQrPayload("home-wifi", "a,b12345");
    TEST_ASSERT_EQUAL_STRING("WIFI:T:WPA;S:home-wifi;P:a\\,b12345;;", payload.c_str());
}

static void test_double_quote_in_ssid_is_escaped() {
    std::string payload = buildWifiQrPayload("a\"b", "pass1234");
    TEST_ASSERT_EQUAL_STRING("WIFI:T:WPA;S:a\\\"b;P:pass1234;;", payload.c_str());
}

static void test_colon_in_password_is_escaped() {
    std::string payload = buildWifiQrPayload("home-wifi", "a:b12345");
    TEST_ASSERT_EQUAL_STRING("WIFI:T:WPA;S:home-wifi;P:a\\:b12345;;", payload.c_str());
}

static void test_backslash_in_ssid_is_escaped() {
    std::string payload = buildWifiQrPayload("a\\b", "pass1234");
    TEST_ASSERT_EQUAL_STRING("WIFI:T:WPA;S:a\\\\b;P:pass1234;;", payload.c_str());
}

static void test_multiple_special_characters_are_all_escaped() {
    std::string payload = buildWifiQrPayload("s;s,s\"", "p:p\\p");
    TEST_ASSERT_EQUAL_STRING("WIFI:T:WPA;S:s\\;s\\,s\\\";P:p\\:p\\\\p;;", payload.c_str());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_basic_payload_has_wpa_ssid_and_password);
    RUN_TEST(test_open_network_uses_nopass_and_omits_password_field);
    RUN_TEST(test_semicolon_in_ssid_is_escaped);
    RUN_TEST(test_comma_in_password_is_escaped);
    RUN_TEST(test_double_quote_in_ssid_is_escaped);
    RUN_TEST(test_colon_in_password_is_escaped);
    RUN_TEST(test_backslash_in_ssid_is_escaped);
    RUN_TEST(test_multiple_special_characters_are_all_escaped);
    return UNITY_END();
}
