#include "wifi_scan.h"

#include <algorithm>

std::vector<WifiScanEntry> dedupeSortWifiScan(const std::vector<WifiScanEntry>& scan) {
    std::vector<WifiScanEntry> out;
    for (const auto& e : scan) {
        if (e.ssid.empty()) continue;
        auto found = std::find_if(out.begin(), out.end(),
                                  [&](const WifiScanEntry& o) { return o.ssid == e.ssid; });
        if (found == out.end()) {
            out.push_back(e);
        } else if (e.rssi > found->rssi) {
            *found = e;  // 同名 AP(メッシュ等)は最強のものを代表にする
        }
    }
    std::stable_sort(out.begin(), out.end(),
                     [](const WifiScanEntry& a, const WifiScanEntry& b) { return a.rssi > b.rssi; });
    return out;
}
