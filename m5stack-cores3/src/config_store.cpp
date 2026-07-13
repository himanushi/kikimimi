#include "config_store.h"

#include <Preferences.h>

namespace {
constexpr const char* NVS_NAMESPACE = "kikimimi";
}

StoredConfig configLoad() {
    Preferences prefs;
    StoredConfig config;
    if (!prefs.begin(NVS_NAMESPACE, /*readOnly=*/true)) return config;  // 未作成なら空のまま
    config.ssid = prefs.getString("ssid", "");
    config.pass = prefs.getString("pass", "");
    config.apiKey = prefs.getString("apikey", "");
    prefs.end();
    return config;
}

bool configSave(const StoredConfig& config) {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, /*readOnly=*/false)) return false;
    // putString は書き込んだバイト数を返す(空文字の正常書き込みは 0 なので pass は空を許容)
    size_t ssidWritten = prefs.putString("ssid", config.ssid);
    size_t passWritten = prefs.putString("pass", config.pass);
    size_t keyWritten = prefs.putString("apikey", config.apiKey);
    prefs.end();
    return ssidWritten > 0 && (config.pass.length() == 0 || passWritten > 0) && keyWritten > 0;
}
