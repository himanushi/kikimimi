#include "config_store.h"

#include <Preferences.h>

namespace {
constexpr const char* NVS_NAMESPACE = "kikimimi";
}

StoredConfig configLoad() {
    Preferences prefs;
    StoredConfig config;
    if (!prefs.begin(NVS_NAMESPACE, /*readOnly=*/true)) return config;  // 未作成なら空のまま
    std::string wifiJson = prefs.getString("wifilist", "").c_str();
    std::string legacySsid = prefs.getString("ssid", "").c_str();
    std::string legacyPass = prefs.getString("pass", "").c_str();
    config.apiKey = prefs.getString("apikey", "");
    config.volume = prefs.getUChar("volume", CONFIG_DEFAULT_VOLUME);
    prefs.end();

    config.wifiList = migrateLegacyWifi(deserializeWifiList(wifiJson), legacySsid, legacyPass);
    return config;
}

bool configSave(const std::vector<WifiCredential>& wifiList, const String& apiKey) {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, /*readOnly=*/false)) return false;
    // putString は書き込んだバイト数を返す(空文字の正常書き込みは 0 なので許容する)
    size_t wifiWritten = prefs.putString("wifilist", serializeWifiList(wifiList).c_str());
    size_t keyWritten = prefs.putString("apikey", apiKey);
    // 旧・単一 ssid/pass 形式を消す。残したままだと configLoad() の移行処理が、
    // 一覧から削除したはずのエントリを毎回復活させてしまう
    prefs.remove("ssid");
    prefs.remove("pass");
    prefs.end();
    return wifiWritten > 0 && keyWritten > 0;
}

bool configSaveVolume(uint8_t volume) {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, /*readOnly=*/false)) return false;
    size_t written = prefs.putUChar("volume", volume);
    prefs.end();
    return written > 0;
}
