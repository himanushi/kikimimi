// 保存済み WiFi 認証情報リスト(最大 5 件)の JSON シリアライズ・デシリアライズと、
// 旧・単一 ssid/pass 形式からの移行、追加・削除ロジック(issue 00011)。
// デバイス非依存の純関数(native 環境でテストする)
#pragma once

#include <string>
#include <vector>

constexpr size_t WIFI_LIST_MAX_ENTRIES = 5;

struct WifiCredential {
    std::string ssid;
    std::string pass;
};

// NVS への保存用に JSON 文字列化する
std::string serializeWifiList(const std::vector<WifiCredential>& list);

// JSON 文字列からリストを復元する。空・壊れた JSON は空リストを返す
std::vector<WifiCredential> deserializeWifiList(const std::string& json);

// 旧形式(単一 ssid/pass)からの移行: legacySsid が非空かつ list に同名 SSID が無ければ 1 件追加する。
// list が最大件数のときは追加しない(移行より既存の複数登録を優先する)
std::vector<WifiCredential> migrateLegacyWifi(const std::vector<WifiCredential>& list,
                                               const std::string& legacySsid,
                                               const std::string& legacyPass);

struct WifiListAddResult {
    bool ok = false;
    std::string error;  // ok == false のとき、表示用メッセージ
    std::vector<WifiCredential> list;  // ok == true のとき、更新後のリスト
};

// 同名 SSID があれば置き換え(パスワード更新)、なければ追加する。
// 新規追加で件数が WIFI_LIST_MAX_ENTRIES を超える場合は拒否する
WifiListAddResult addOrUpdateWifiCredential(const std::vector<WifiCredential>& list,
                                            const WifiCredential& cred);

// 指定 SSID のエントリを取り除いたリストを返す(存在しなければそのまま)
std::vector<WifiCredential> removeWifiCredential(const std::vector<WifiCredential>& list,
                                                 const std::string& ssid);
