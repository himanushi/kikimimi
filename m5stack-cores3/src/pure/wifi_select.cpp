#include "wifi_select.h"

#include <algorithm>

std::vector<WifiCandidate> selectWifiCandidates(const std::vector<WifiCredential>& saved,
                                                const std::vector<WifiScanEntry>& scanResults) {
    std::vector<WifiCandidate> out;
    for (const auto& cred : saved) {
        bool found = false;
        int bestRssi = 0;
        for (const auto& entry : scanResults) {
            if (entry.ssid != cred.ssid) continue;
            if (!found || entry.rssi > bestRssi) bestRssi = entry.rssi;
            found = true;
        }
        if (found) out.push_back({cred, bestRssi});
    }
    std::stable_sort(out.begin(), out.end(),
                     [](const WifiCandidate& a, const WifiCandidate& b) { return a.rssi > b.rssi; });
    return out;
}
