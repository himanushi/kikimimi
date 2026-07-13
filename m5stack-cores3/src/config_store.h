// NVS(Preferences)への設定の保存・読み出し。WiFi は最大 5 件を JSON でリスト保存する(issue 00011)。
// 旧・単一 ssid/pass 形式で保存済みの場合は configLoad() 時にリストへ移行する
#pragma once

#include <Arduino.h>

#include <vector>

#include "pure/wifi_list.h"

// 未保存時のデフォルト音量(M5Unified の既定値相当)
constexpr uint8_t CONFIG_DEFAULT_VOLUME = 128;

struct StoredConfig {
    std::vector<WifiCredential> wifiList;
    String apiKey;
    uint8_t volume = CONFIG_DEFAULT_VOLUME;

    bool hasWifi() const { return !wifiList.empty(); }
};

// 音量のみ保存する(WiFi/API キーは変更しない)
bool configSaveVolume(uint8_t volume);

StoredConfig configLoad();
// WiFi リストと API キーを保存する。NVS への書き込みに失敗したら false(呼び出し側がユーザーへエラー表示する)
bool configSave(const std::vector<WifiCredential>& wifiList, const String& apiKey);
