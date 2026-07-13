// kikimimi CoreS3 ファーム: 起動 → 保存済み WiFi へ接続、未設定・接続失敗ならセットアップポータル
#include <M5Unified.h>
#include <WiFi.h>

#include "config_store.h"
#include "portal.h"
#include "pure/home_screen.h"
#include "pure/portal_screen.h"
#include "pure/usage_cost.h"
#include "pure/wifi_qr.h"
#include "realtime_client.h"

namespace {

enum class Mode { PORTAL, CONNECTED };
Mode mode = Mode::PORTAL;

constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr const char* SESSION_INSTRUCTIONS = "あなたは kikimimi という名前の相棒です。短く日本語で応答してください。";

// JST 固定(海外での利用を想定しないため timezone 設定はしない)
constexpr long NTP_GMT_OFFSET_SEC = 9 * 3600;
constexpr const char* NTP_SERVER_PRIMARY = "ntp.nict.jp";
constexpr const char* NTP_SERVER_FALLBACK = "pool.ntp.org";

constexpr uint32_t COLOR_BG = 0x0A0E14;
constexpr uint32_t COLOR_ACCENT = 0x00C2A8;
constexpr uint32_t COLOR_ACCENT_DIM = 0x123B38;
constexpr uint32_t COLOR_TEXT = 0xE6E6E6;
constexpr uint32_t COLOR_SUBTEXT = 0x6E7681;

constexpr uint32_t HOME_REFRESH_INTERVAL_MS = 30000;  // バッテリー・時計の定期再描画間隔

String storedApiKey;
String errorMessage;
double sessionCostJpy = 0.0;
int lastDrawnMinute = -1;         // -1: 未同期 or 未描画
uint32_t lastHomeRefreshMs = 0;

PortalInfo portalInfo;
String portalReason;
int lastPortalStationCount = -1;  // -1: 未描画(初回は必ず描画させる)

void drawLines(std::initializer_list<String> lines) {
    auto& d = M5.Display;
    d.clear(TFT_BLACK);
    d.setFont(&fonts::efontJA_16);
    d.setTextColor(TFT_WHITE, TFT_BLACK);
    int y = 16;
    for (const auto& line : lines) {
        d.setCursor(12, y);
        d.print(line);
        y += 28;
    }
}

String currentClockLabel() {
    struct tm t;
    if (!getLocalTime(&t, 0)) return "--:--";
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", t.tm_hour, t.tm_min);
    return String(buf);
}

// ホーム画面(対話開始ボタン・設定ボタン・バッテリー・時刻)。RealtimeState::Idle のときのみ表示する
void drawHomeScreen() {
    auto& d = M5.Display;
    int w = d.width();
    int h = d.height();
    HomeLayout layout = homeScreenLayout(w, h);

    d.startWrite();
    d.fillScreen(COLOR_BG);
    d.setFont(&fonts::efontJA_16);

    d.setTextColor(COLOR_SUBTEXT, COLOR_BG);
    d.setCursor(12, 12);
    d.printf("%d%%", M5.Power.getBatteryLevel());

    String clock = currentClockLabel();
    int clockWidth = d.textWidth(clock);
    d.setCursor(w - clockWidth - 12, 12);
    d.print(clock);

    const HomeRect& tb = layout.talkButton;
    d.fillRoundRect(tb.x, tb.y, tb.w, tb.h, tb.w / 6, COLOR_ACCENT);
    String talkLabel = "はなす";
    int talkLabelWidth = d.textWidth(talkLabel);
    d.setTextColor(COLOR_BG, COLOR_ACCENT);
    d.setCursor(tb.x + (tb.w - talkLabelWidth) / 2, tb.y + tb.h / 2 - 8);
    d.print(talkLabel);

    const HomeRect& sb = layout.settingsButton;
    d.fillRoundRect(sb.x, sb.y, sb.w, sb.h, sb.w / 4, COLOR_ACCENT_DIM);
    String settingsLabel = "設定";
    int settingsLabelWidth = d.textWidth(settingsLabel);
    d.setTextColor(COLOR_TEXT, COLOR_ACCENT_DIM);
    d.setCursor(sb.x + (sb.w - settingsLabelWidth) / 2, sb.y + sb.h / 2 - 8);
    d.print(settingsLabel);

    d.endWrite();
}

// ホーム画面を描画し、定期再描画の基準(分・タイマー)を更新する
void drawHomeScreenNow() {
    drawHomeScreen();
    struct tm t;
    lastDrawnMinute = getLocalTime(&t, 0) ? t.tm_min : -1;
    lastHomeRefreshMs = millis();
}

// loop() から毎回呼ぶ。分が変わった、または HOME_REFRESH_INTERVAL_MS 経過したときだけ再描画する
void updateHomeScreenPeriodic() {
    struct tm t;
    int minute = getLocalTime(&t, 0) ? t.tm_min : -1;
    bool refreshDue = millis() - lastHomeRefreshMs >= HOME_REFRESH_INTERVAL_MS;
    if (minute == lastDrawnMinute && !refreshDue) return;
    drawHomeScreenNow();
}

bool connectWifi(const StoredConfig& config) {
    drawLines({"WiFi に接続中...", "SSID: " + config.ssid});
    Serial.printf("[wifi] connecting to %s\n", config.ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.ssid.c_str(), config.pass.c_str());
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
        delay(200);
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.printf("[wifi] connect failed (status=%d)\n", WiFi.status());
        WiFi.disconnect(true);
        return false;
    }
    Serial.printf("[wifi] connected, ip=%s\n", WiFi.localIP().toString().c_str());
    return true;
}

// 左: テキスト情報 / 右: QR(QR は AP 接続用)
void drawApJoinScreen(const String& reason, const PortalInfo& info) {
    drawLines({reason, "スマホで WiFi に接続:", "  " + info.apName,
              "  パスワード: " + info.apPass, "", "ブラウザで開く:", "  " + info.url});
    // テキスト行(x=12 起点)と QR が重ならないよう、QR は画面右端寄せで幅を絞る
    std::string qrPayload = buildWifiQrPayload(info.apName.c_str(), info.apPass.c_str());
    M5.Display.qrcode(qrPayload.c_str(), 210, 50, 100);
}

// AP 接続後の案内。QR のペイロードは URL 文字列そのまま(秘密情報は含まれない)
void drawUrlGuideScreen(const PortalInfo& info) {
    drawLines({"接続しました", "ブラウザで設定ページを開く:", "  " + info.url, "", "QR を読み取ってください"});
    M5.Display.qrcode(info.url.c_str(), 210, 50, 100);
}

// 接続台数に応じた画面を、直前の描画から変化があったときだけ描き直す
void updatePortalScreen(int stationCount) {
    if (stationCount == lastPortalStationCount) return;
    lastPortalStationCount = stationCount;
    switch (portalScreenForStationCount(stationCount)) {
        case PortalScreen::ApJoin: drawApJoinScreen(portalReason, portalInfo); break;
        case PortalScreen::UrlGuide: drawUrlGuideScreen(portalInfo); break;
    }
}

void startPortal(const String& reason) {
    portalInfo = portalStart();
    portalReason = reason;
    // AP パスワードはシリアルに出さない(画面にのみ表示)
    Serial.printf("[portal] started ap=%s url=%s\n", portalInfo.apName.c_str(), portalInfo.url.c_str());
    lastPortalStationCount = -1;
    updatePortalScreen(portalStationCount());
    mode = Mode::PORTAL;
}

String realtimeStateLabel(RealtimeState state) {
    switch (state) {
        case RealtimeState::Idle: return "タップで対話を開始";
        case RealtimeState::Connecting: return "接続中...";
        case RealtimeState::Listening: return "聞いています(タップで終了)";
        case RealtimeState::Thinking: return "考えています...";
        case RealtimeState::Speaking: return "話しています...";
        case RealtimeState::ErrorState: return "エラー(タップで再試行)";
    }
    return "";
}

void drawRealtimeScreen() {
    String statusLine = realtimeStateLabel(realtimeCurrentState()) + "  " +
                         usage_cost::formatJpyLabel(sessionCostJpy).c_str();
    drawLines({statusLine, "", errorMessage});
}

// Idle はホーム画面、それ以外(接続中・対話中・エラー)は従来の状態表示画面
void drawForCurrentState() {
    if (realtimeCurrentState() == RealtimeState::Idle) {
        drawHomeScreenNow();
    } else {
        drawRealtimeScreen();
    }
}

RealtimeCallbacks buildRealtimeCallbacks() {
    RealtimeCallbacks cb;
    cb.onStateChanged = [](RealtimeState state) {
        if (state == RealtimeState::Listening) errorMessage = "";
        drawForCurrentState();
    };
    cb.onError = [](const String& message) {
        errorMessage = message;
        drawRealtimeScreen();
    };
    cb.onCostUpdated = [](double jpy) {
        sessionCostJpy = jpy;
        drawRealtimeScreen();
    };
    return cb;
}

void toggleRealtimeConnection() {
    RealtimeState state = realtimeCurrentState();
    if (state == RealtimeState::Idle || state == RealtimeState::ErrorState) {
        Serial.printf("[realtime] free heap=%u free psram=%u\n", ESP.getFreeHeap(),
                     ESP.getFreePsram());
        realtimeConnect(storedApiKey, SESSION_INSTRUCTIONS, buildRealtimeCallbacks());
    } else {
        realtimeDisconnect();
        drawForCurrentState();
    }
}

}  // namespace

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    Serial.begin(115200);

    StoredConfig config = configLoad();
    if (!config.hasWifi()) {
        startPortal("未設定です");
        return;
    }
    if (!connectWifi(config)) {
        startPortal("WiFi 接続に失敗しました");
        return;
    }
    configTime(NTP_GMT_OFFSET_SEC, 0, NTP_SERVER_PRIMARY, NTP_SERVER_FALLBACK);
    mode = Mode::CONNECTED;
    storedApiKey = config.apiKey;
    drawForCurrentState();
}

void loop() {
    M5.update();
    if (mode == Mode::PORTAL) {
        updatePortalScreen(portalStationCount());
        if (portalLoop()) {
            Serial.println("[portal] config saved, restarting");
            ESP.restart();
        }
    }
    if (mode == Mode::CONNECTED) {
        realtimeLoop();
        RealtimeState state = realtimeCurrentState();
        if (state == RealtimeState::Idle) {
            updateHomeScreenPeriodic();
            auto touch = M5.Touch.getDetail();
            if (touch.wasPressed()) {
                switch (homeScreenHitTest(M5.Display.width(), M5.Display.height(), touch.x, touch.y)) {
                    case HomeTap::Talk: toggleRealtimeConnection(); break;
                    case HomeTap::Settings: startPortal("設定"); break;
                    case HomeTap::None: break;
                }
            }
        } else if (M5.Touch.getDetail().wasPressed()) {
            toggleRealtimeConnection();
        }
    }
    delay(10);
}
