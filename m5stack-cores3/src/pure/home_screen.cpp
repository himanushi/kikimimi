#include "home_screen.h"

namespace {

bool contains(const HomeRect& r, int x, int y) {
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

}  // namespace

HomeLayout homeScreenLayout(int screenWidth, int screenHeight) {
    HomeLayout layout;

    int talkSize = screenHeight * 3 / 5;
    layout.talkButton = {
        (screenWidth - talkSize) / 2,
        (screenHeight - talkSize) / 2 + screenHeight / 20,
        talkSize,
        talkSize,
    };

    int settingsSize = screenHeight / 6;
    int margin = screenHeight / 24;
    layout.settingsButton = {
        screenWidth - settingsSize - margin,
        screenHeight - settingsSize - margin,
        settingsSize,
        settingsSize,
    };

    return layout;
}

HomeTap homeScreenHitTest(int screenWidth, int screenHeight, int x, int y) {
    HomeLayout layout = homeScreenLayout(screenWidth, screenHeight);
    if (contains(layout.settingsButton, x, y)) return HomeTap::Settings;
    if (contains(layout.talkButton, x, y)) return HomeTap::Talk;
    return HomeTap::None;
}
