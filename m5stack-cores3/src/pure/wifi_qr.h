// WiFi 接続用 QR コード(`WIFI:T:WPA;S:<ssid>;P:<pass>;;`)のペイロード生成。
// デバイス非依存の純関数(native 環境でテストする)
#pragma once

#include <string>

// ssid・pass 中の `\ ; , " :` はエスケープしてから WIFI: フォーマットへ埋め込む。
// pass が空(オープンネットワーク)の場合は T:nopass; を用いる。
std::string buildWifiQrPayload(const std::string& ssid, const std::string& pass);
