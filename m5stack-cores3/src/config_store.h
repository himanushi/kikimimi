// NVS(Preferences)への設定の保存・読み出し。WiFi は 1 件のみ保存(同キー上書き)
#pragma once

#include <Arduino.h>

// 未保存時のデフォルト音量(M5Unified の既定値相当)
constexpr uint8_t CONFIG_DEFAULT_VOLUME = 128;

struct StoredConfig {
    String ssid;
    String pass;
    String apiKey;
    uint8_t volume = CONFIG_DEFAULT_VOLUME;

    bool hasWifi() const { return ssid.length() > 0; }
};

// 音量のみ保存する(WiFi/API キーは変更しない)
bool configSaveVolume(uint8_t volume);

StoredConfig configLoad();
// NVS への書き込みに失敗したら false(呼び出し側がユーザーへエラー表示する)
bool configSave(const StoredConfig& config);
