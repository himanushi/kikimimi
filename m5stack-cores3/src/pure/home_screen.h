// ホーム画面(対話開始ボタン・設定ボタン)のレイアウトとタップ判定(issue 00008)。
// デバイス非依存(native 環境でテストする)
#pragma once

struct HomeRect {
    int x, y, w, h;
};

struct HomeLayout {
    HomeRect talkButton;      // 中央の大きい対話開始ボタン
    HomeRect settingsButton;  // 隅の小さい設定ボタン
};

enum class HomeTap {
    None,
    Talk,
    Settings,
};

HomeLayout homeScreenLayout(int screenWidth, int screenHeight);
HomeTap homeScreenHitTest(int screenWidth, int screenHeight, int x, int y);
