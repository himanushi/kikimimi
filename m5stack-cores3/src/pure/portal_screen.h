// セットアップポータルの表示画面判定(issue 00005)。
// AP への接続台数から、表示すべき画面(AP 接続案内 / 設定 URL 案内)を決める純関数。
// デバイス非依存(native 環境でテストする)
#pragma once

enum class PortalScreen {
    ApJoin,    // 接続 0 台: AP 接続用 QR を表示
    UrlGuide,  // 接続 1 台以上: 設定 URL 案内(URL テキスト + URL QR)を表示
};

PortalScreen portalScreenForStationCount(int stationCount);
