// 保存済み WiFi リストとスキャン結果から、RSSI 降順の接続候補を選ぶ(issue 00011 受け入れ基準)。
// デバイス非依存の純関数(native 環境でテストする)
#pragma once

#include <vector>

#include "wifi_list.h"
#include "wifi_scan.h"

struct WifiCandidate {
    WifiCredential credential;
    int rssi;
};

// saved のうち scanResults に見つかったものだけを RSSI 降順で返す。
// 一致しないものは除外し、同一 SSID が複数 AP で見える場合は最も強い RSSI を採用する
std::vector<WifiCandidate> selectWifiCandidates(const std::vector<WifiCredential>& saved,
                                                const std::vector<WifiScanEntry>& scanResults);
