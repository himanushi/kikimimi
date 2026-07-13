// セットアップポータル: SoftAP + captive DNS + 設定フォーム(SSID / パスワード / API キー)
#pragma once

#include <Arduino.h>

// SoftAP と Web サーバを起動する。表示用に AP 名・AP パスワード・URL を返す
struct PortalInfo {
    String apName;
    String apPass;  // 起動ごとの乱数(本体画面にのみ表示。無線区間の API キー保護のため WPA2 必須)
    String url;
};
PortalInfo portalStart();

// loop() から毎回呼ぶ。フォーム保存後は true を返し続ける(呼び出し側が再起動する)
bool portalLoop();
