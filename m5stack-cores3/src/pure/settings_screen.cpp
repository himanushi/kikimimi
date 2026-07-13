#include "settings_screen.h"

#include <algorithm>

namespace {

bool contains(const SettingsRect& r, int x, int y) {
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

}  // namespace

SettingsLayout settingsScreenLayout(int screenWidth, int screenHeight) {
    SettingsLayout layout;

    int margin = screenHeight / 24;
    int rowHeight = screenHeight / 6;
    int volumeButtonWidth = screenWidth / 4;
    int volumeRowY = screenHeight * 2 / 5;

    layout.volumeDownButton = {screenWidth / 6, volumeRowY, volumeButtonWidth, rowHeight};
    layout.volumeUpButton = {screenWidth - screenWidth / 6 - volumeButtonWidth, volumeRowY, volumeButtonWidth,
                              rowHeight};

    layout.wifiSetupButton = {margin, volumeRowY + rowHeight + margin, screenWidth - margin * 2, rowHeight};

    layout.backButton = {0, screenHeight - rowHeight, screenWidth / 2, rowHeight};

    return layout;
}

SettingsTap settingsScreenHitTest(int screenWidth, int screenHeight, int x, int y) {
    SettingsLayout layout = settingsScreenLayout(screenWidth, screenHeight);
    if (contains(layout.volumeDownButton, x, y)) return SettingsTap::VolumeDown;
    if (contains(layout.volumeUpButton, x, y)) return SettingsTap::VolumeUp;
    if (contains(layout.wifiSetupButton, x, y)) return SettingsTap::WifiSetup;
    if (contains(layout.backButton, x, y)) return SettingsTap::Back;
    return SettingsTap::None;
}

int settingsVolumeIncrease(int currentVolume) {
    return std::min(255, currentVolume + SETTINGS_VOLUME_STEP);
}

int settingsVolumeDecrease(int currentVolume) {
    return std::max(0, currentVolume - SETTINGS_VOLUME_STEP);
}
