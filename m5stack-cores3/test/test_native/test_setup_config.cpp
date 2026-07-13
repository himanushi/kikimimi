// セットアップポータルの入力検証(issue 00001 受け入れ基準)のテスト。
// 仕様: SSID は必須・32 バイト以内。パスワードは空(オープン)または WPA2 の 8〜63 文字。
// API キーはブラウザからのコピペを想定し、前後の空白・改行を除去したうえで形式を確認する。
#include <unity.h>

#include "pure/setup_config.h"

void setUp() {}
void tearDown() {}

static const char* VALID_KEY = "sk-proj-abcdefghijklmnopqrstuvwxyz0123456789";

static void test_valid_input_is_accepted_and_trimmed() {
    SetupValidation r = validateSetupInput("  home-wifi  ", "password123",
                                           std::string("  ") + VALID_KEY + " \r\n");
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("home-wifi", r.config.ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("password123", r.config.pass.c_str());
    TEST_ASSERT_EQUAL_STRING(VALID_KEY, r.config.apiKey.c_str());
}

static void test_empty_ssid_is_rejected() {
    SetupValidation r = validateSetupInput("   ", "password123", VALID_KEY);
    TEST_ASSERT_FALSE(r.ok);
    TEST_ASSERT_FALSE(r.error.empty());
}

static void test_ssid_longer_than_32_bytes_is_rejected() {
    SetupValidation r = validateSetupInput(std::string(33, 'a'), "password123", VALID_KEY);
    TEST_ASSERT_FALSE(r.ok);
}

static void test_open_network_empty_password_is_accepted() {
    SetupValidation r = validateSetupInput("home-wifi", "", VALID_KEY);
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL_STRING("", r.config.pass.c_str());
}

static void test_password_shorter_than_wpa2_minimum_is_rejected() {
    SetupValidation r = validateSetupInput("home-wifi", "short12", VALID_KEY);
    TEST_ASSERT_FALSE(r.ok);
}

static void test_password_at_wpa2_minimum_is_accepted() {
    SetupValidation r = validateSetupInput("home-wifi", "exactly8", VALID_KEY);
    TEST_ASSERT_TRUE(r.ok);
}

static void test_api_key_at_maximum_length_is_accepted() {
    SetupValidation r = validateSetupInput("home-wifi", "password123",
                                           "sk-" + std::string(509, 'a'));
    TEST_ASSERT_TRUE(r.ok);
}

static void test_password_longer_than_wpa2_maximum_is_rejected() {
    SetupValidation r = validateSetupInput("home-wifi", std::string(64, 'p'), VALID_KEY);
    TEST_ASSERT_FALSE(r.ok);
}

static void test_api_key_without_sk_prefix_is_rejected() {
    SetupValidation r = validateSetupInput("home-wifi", "password123",
                                           "proj-abcdefghijklmnopqrstuvwxyz0123456789");
    TEST_ASSERT_FALSE(r.ok);
}

static void test_too_short_api_key_is_rejected() {
    SetupValidation r = validateSetupInput("home-wifi", "password123", "sk-abc");
    TEST_ASSERT_FALSE(r.ok);
}

static void test_api_key_with_inner_whitespace_is_rejected() {
    SetupValidation r = validateSetupInput("home-wifi", "password123",
                                           "sk-proj-abcdef ghijklmnopqrstuvwxyz");
    TEST_ASSERT_FALSE(r.ok);
}

static void test_empty_api_key_is_rejected() {
    SetupValidation r = validateSetupInput("home-wifi", "password123", "");
    TEST_ASSERT_FALSE(r.ok);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_valid_input_is_accepted_and_trimmed);
    RUN_TEST(test_empty_ssid_is_rejected);
    RUN_TEST(test_ssid_longer_than_32_bytes_is_rejected);
    RUN_TEST(test_open_network_empty_password_is_accepted);
    RUN_TEST(test_password_shorter_than_wpa2_minimum_is_rejected);
    RUN_TEST(test_password_at_wpa2_minimum_is_accepted);
    RUN_TEST(test_api_key_at_maximum_length_is_accepted);
    RUN_TEST(test_password_longer_than_wpa2_maximum_is_rejected);
    RUN_TEST(test_api_key_without_sk_prefix_is_rejected);
    RUN_TEST(test_too_short_api_key_is_rejected);
    RUN_TEST(test_api_key_with_inner_whitespace_is_rejected);
    RUN_TEST(test_empty_api_key_is_rejected);
    return UNITY_END();
}
