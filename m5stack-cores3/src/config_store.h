// NVS(Preferences)への設定の保存・読み出し。WiFi は 1 件のみ保存(同キー上書き)
#pragma once

#include <Arduino.h>

struct StoredConfig {
    String ssid;
    String pass;
    String apiKey;

    bool hasWifi() const { return ssid.length() > 0; }
};

StoredConfig configLoad();
// NVS への書き込みに失敗したら false(呼び出し側がユーザーへエラー表示する)
bool configSave(const StoredConfig& config);
