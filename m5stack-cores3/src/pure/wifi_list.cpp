#include "wifi_list.h"

#include <ArduinoJson.h>

std::string serializeWifiList(const std::vector<WifiCredential>& list) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (const auto& cred : list) {
        JsonObject obj = arr.add<JsonObject>();
        obj["ssid"] = cred.ssid;
        obj["pass"] = cred.pass;
    }
    std::string out;
    serializeJson(doc, out);
    return out;
}

std::vector<WifiCredential> deserializeWifiList(const std::string& json) {
    std::vector<WifiCredential> out;
    if (json.empty()) return out;
    JsonDocument doc;
    if (deserializeJson(doc, json)) return out;  // 壊れた JSON は空リストのまま
    JsonArray arr = doc.as<JsonArray>();
    if (arr.isNull()) return out;
    for (JsonObject obj : arr) {
        WifiCredential cred;
        cred.ssid = obj["ssid"] | "";
        cred.pass = obj["pass"] | "";
        out.push_back(cred);
    }
    return out;
}

namespace {
bool containsSsid(const std::vector<WifiCredential>& list, const std::string& ssid) {
    for (const auto& c : list) {
        if (c.ssid == ssid) return true;
    }
    return false;
}
}  // namespace

std::vector<WifiCredential> migrateLegacyWifi(const std::vector<WifiCredential>& list,
                                               const std::string& legacySsid,
                                               const std::string& legacyPass) {
    if (legacySsid.empty()) return list;
    if (containsSsid(list, legacySsid)) return list;
    if (list.size() >= WIFI_LIST_MAX_ENTRIES) return list;
    std::vector<WifiCredential> out = list;
    out.push_back({legacySsid, legacyPass});
    return out;
}

WifiListAddResult addOrUpdateWifiCredential(const std::vector<WifiCredential>& list,
                                            const WifiCredential& cred) {
    WifiListAddResult r;
    std::vector<WifiCredential> out = list;
    for (auto& c : out) {
        if (c.ssid == cred.ssid) {
            c.pass = cred.pass;
            r.ok = true;
            r.list = out;
            return r;
        }
    }
    if (out.size() >= WIFI_LIST_MAX_ENTRIES) {
        r.ok = false;
        r.error = "WiFi は " + std::to_string(WIFI_LIST_MAX_ENTRIES) + " 件まで登録できます";
        return r;
    }
    out.push_back(cred);
    r.ok = true;
    r.list = out;
    return r;
}

std::vector<WifiCredential> removeWifiCredential(const std::vector<WifiCredential>& list,
                                                 const std::string& ssid) {
    std::vector<WifiCredential> out;
    for (const auto& c : list) {
        if (c.ssid != ssid) out.push_back(c);
    }
    return out;
}
