// 設定画面の「WiFi / API キー設定」導線判定(issue 00010)。
// WiFi 接続済み(STA)なら AP に切り替えず STA の URL 案内を出す。未接続なら従来の AP ポータル。
// デバイス非依存(native 環境でテストする)
#pragma once

enum class SettingsEntryAction {
    ShowStaUrlGuide,  // WiFi 接続済み: STA の IP の URL 案内を表示
    StartApPortal,    // WiFi 未接続: 従来の AP ポータルにフォールバック
};

SettingsEntryAction settingsEntryAction(bool wifiConnected);
