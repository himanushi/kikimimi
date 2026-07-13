// 設定画面(音量・WiFi 設定導線・戻る)のレイアウトとタップ判定(issue 00012)。
// デバイス非依存(native 環境でテストする)
#pragma once

struct SettingsRect {
    int x, y, w, h;
};

struct SettingsLayout {
    SettingsRect volumeDownButton;
    SettingsRect volumeUpButton;
    SettingsRect wifiSetupButton;
    SettingsRect backButton;
};

enum class SettingsTap {
    None,
    VolumeDown,
    VolumeUp,
    WifiSetup,
    Back,
};

// 音量増減の 1 段階。0..255 の範囲でクランプする
constexpr int SETTINGS_VOLUME_STEP = 32;

SettingsLayout settingsScreenLayout(int screenWidth, int screenHeight);
SettingsTap settingsScreenHitTest(int screenWidth, int screenHeight, int x, int y);

int settingsVolumeIncrease(int currentVolume);
int settingsVolumeDecrease(int currentVolume);
