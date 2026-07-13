// WiFi 認証情報リストの JSON 化・復元・旧形式移行・追加/削除(issue 00011 受け入れ基準)のテスト。
#include <unity.h>

#include "pure/wifi_list.h"

void setUp() {}
void tearDown() {}

static void test_serialize_then_deserialize_roundtrips() {
    std::vector<WifiCredential> list = {{"home", "pass1234"}, {"office", ""}};
    std::string json = serializeWifiList(list);
    std::vector<WifiCredential> out = deserializeWifiList(json);
    TEST_ASSERT_EQUAL(2, out.size());
    TEST_ASSERT_EQUAL_STRING("home", out[0].ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("pass1234", out[0].pass.c_str());
    TEST_ASSERT_EQUAL_STRING("office", out[1].ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("", out[1].pass.c_str());
}

static void test_deserialize_empty_string_returns_empty_list() {
    std::vector<WifiCredential> out = deserializeWifiList("");
    TEST_ASSERT_EQUAL(0, out.size());
}

static void test_deserialize_broken_json_returns_empty_list() {
    std::vector<WifiCredential> out = deserializeWifiList("{not json");
    TEST_ASSERT_EQUAL(0, out.size());
}

static void test_migrate_legacy_adds_entry_when_list_empty() {
    std::vector<WifiCredential> out = migrateLegacyWifi({}, "home", "pass1234");
    TEST_ASSERT_EQUAL(1, out.size());
    TEST_ASSERT_EQUAL_STRING("home", out[0].ssid.c_str());
    TEST_ASSERT_EQUAL_STRING("pass1234", out[0].pass.c_str());
}

static void test_migrate_legacy_does_nothing_when_ssid_empty() {
    std::vector<WifiCredential> out = migrateLegacyWifi({}, "", "");
    TEST_ASSERT_EQUAL(0, out.size());
}

static void test_migrate_legacy_does_not_duplicate_existing_ssid() {
    std::vector<WifiCredential> list = {{"home", "existing-pass"}};
    std::vector<WifiCredential> out = migrateLegacyWifi(list, "home", "legacy-pass");
    TEST_ASSERT_EQUAL(1, out.size());
    TEST_ASSERT_EQUAL_STRING("existing-pass", out[0].pass.c_str());
}

static void test_migrate_legacy_skipped_when_list_full() {
    std::vector<WifiCredential> list = {{"a", ""}, {"b", ""}, {"c", ""}, {"d", ""}, {"e", ""}};
    std::vector<WifiCredential> out = migrateLegacyWifi(list, "legacy", "pw123456");
    TEST_ASSERT_EQUAL(5, out.size());
}

static void test_add_new_credential_appends() {
    std::vector<WifiCredential> list = {{"home", "pass1234"}};
    WifiListAddResult r = addOrUpdateWifiCredential(list, {"office", "office-pass"});
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL(2, r.list.size());
    TEST_ASSERT_EQUAL_STRING("office", r.list[1].ssid.c_str());
}

static void test_add_existing_ssid_replaces_password() {
    std::vector<WifiCredential> list = {{"home", "old-pass"}};
    WifiListAddResult r = addOrUpdateWifiCredential(list, {"home", "new-pass"});
    TEST_ASSERT_TRUE(r.ok);
    TEST_ASSERT_EQUAL(1, r.list.size());
    TEST_ASSERT_EQUAL_STRING("new-pass", r.list[0].pass.c_str());
}

static void test_add_beyond_max_entries_is_rejected() {
    std::vector<WifiCredential> list = {{"a", ""}, {"b", ""}, {"c", ""}, {"d", ""}, {"e", ""}};
    WifiListAddResult r = addOrUpdateWifiCredential(list, {"f", "newpass1"});
    TEST_ASSERT_FALSE(r.ok);
    TEST_ASSERT_EQUAL(5, list.size());
}

static void test_remove_existing_ssid() {
    std::vector<WifiCredential> list = {{"home", "p1"}, {"office", "p2"}};
    std::vector<WifiCredential> out = removeWifiCredential(list, "home");
    TEST_ASSERT_EQUAL(1, out.size());
    TEST_ASSERT_EQUAL_STRING("office", out[0].ssid.c_str());
}

static void test_remove_missing_ssid_is_noop() {
    std::vector<WifiCredential> list = {{"home", "p1"}};
    std::vector<WifiCredential> out = removeWifiCredential(list, "not-here");
    TEST_ASSERT_EQUAL(1, out.size());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_serialize_then_deserialize_roundtrips);
    RUN_TEST(test_deserialize_empty_string_returns_empty_list);
    RUN_TEST(test_deserialize_broken_json_returns_empty_list);
    RUN_TEST(test_migrate_legacy_adds_entry_when_list_empty);
    RUN_TEST(test_migrate_legacy_does_nothing_when_ssid_empty);
    RUN_TEST(test_migrate_legacy_does_not_duplicate_existing_ssid);
    RUN_TEST(test_migrate_legacy_skipped_when_list_full);
    RUN_TEST(test_add_new_credential_appends);
    RUN_TEST(test_add_existing_ssid_replaces_password);
    RUN_TEST(test_add_beyond_max_entries_is_rejected);
    RUN_TEST(test_remove_existing_ssid);
    RUN_TEST(test_remove_missing_ssid_is_noop);
    return UNITY_END();
}
