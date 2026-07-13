// WiFi スキャン結果の整形(空 SSID 除外・重複除去・RSSI 降順ソート、issue 00006)。
// デバイス非依存の純関数(native 環境でテストする)
#pragma once

#include <string>
#include <vector>

struct WifiScanEntry {
    std::string ssid;
    int rssi;  // dBm(0 に近いほど強い)
};

// 空 SSID を除外し、同名 SSID は最も強い RSSI のものを残し、RSSI 降順で返す(設定ページの選択肢用)
std::vector<WifiScanEntry> dedupeSortWifiScan(const std::vector<WifiScanEntry>& scan);
