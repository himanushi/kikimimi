#include "wifi_qr.h"

namespace {

// WIFI: フォーマットで意味を持つ区切り文字はすべてバックスラッシュエスケープする
std::string escaped(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '\\': case ';': case ',': case '"': case ':':
                out += '\\';
                out += c;
                break;
            default:
                out += c;
        }
    }
    return out;
}

}  // namespace

std::string buildWifiQrPayload(const std::string& ssid, const std::string& pass) {
    if (pass.empty()) {
        return "WIFI:T:nopass;S:" + escaped(ssid) + ";;";
    }
    return "WIFI:T:WPA;S:" + escaped(ssid) + ";P:" + escaped(pass) + ";;";
}
