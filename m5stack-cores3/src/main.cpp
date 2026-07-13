// kikimimi CoreS3 ファーム: 起動 → 保存済み WiFi へ接続、未設定・接続失敗ならセットアップポータル
#include <M5Unified.h>
#include <WiFi.h>

#include <algorithm>
#include <vector>

#include "config_store.h"
#include "portal.h"
#include "pure/home_screen.h"
#include "pure/portal_screen.h"
#include "pure/settings_entry.h"
#include "pure/settings_screen.h"
#include "pure/transcript_view.h"
#include "pure/usage_cost.h"
#include "pure/wifi_qr.h"
#include "pure/wifi_scan.h"
#include "pure/wifi_select.h"
#include "realtime_client.h"

namespace {

// SETTINGS はホーム(Idle)からのみ入り、戻る/WiFi 設定でホーム or ポータルへ抜ける。
// SETTINGS_URL_GUIDE は SETTINGS から WiFi 設定済みのときだけ入り、タップで SETTINGS に戻る
enum class Mode { PORTAL, CONNECTED, SETTINGS, SETTINGS_URL_GUIDE };
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
constexpr uint32_t COLOR_TOUCH_MARK = 0x8FA3AD;  // タップ位置に一瞬出す薄い白丸

constexpr int TOUCH_MARK_RADIUS = 14;
constexpr uint32_t TOUCH_MARK_DURATION_MS = 150;

constexpr uint32_t HOME_REFRESH_INTERVAL_MS = 30000;  // バッテリー・時計の定期再描画間隔

// 対話画面のレイアウト。上部: 状態 + 累計金額、下部: 会話テキスト
constexpr int TRANSCRIPT_TOP_Y = 60;
constexpr int TRANSCRIPT_LINE_HEIGHT = 22;
// efontJA_16 は全角 16px 相当。画面幅 320px・左右マージン込みで概算した折り返し文字数
constexpr int TRANSCRIPT_WRAP_WIDTH = 18;

String storedApiKey;
uint8_t currentVolume = CONFIG_DEFAULT_VOLUME;
String errorMessage;
double sessionCostJpy = 0.0;
transcript_view::History transcriptHistory;  // 対話画面の会話履歴。セッション終了(切断)で破棄する
int lastDrawnMinute = -1;         // -1: 未同期 or 未描画
uint32_t lastHomeRefreshMs = 0;

PortalInfo portalInfo;
String portalReason;
int lastPortalStationCount = -1;  // -1: 未描画(初回は必ず描画させる)
PortalInfo staPortalInfo;  // STA 常設の URL 案内用(apName/apPass は使わない)

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

// 設定画面(音量 +/-・WiFi 設定導線・戻る)。ホーム画面と同じダーク + ティール配色
void drawSettingsScreen() {
    auto& d = M5.Display;
    int w = d.width();
    int h = d.height();
    SettingsLayout layout = settingsScreenLayout(w, h);

    d.startWrite();
    d.fillScreen(COLOR_BG);
    d.setFont(&fonts::efontJA_16);

    d.setTextColor(COLOR_TEXT, COLOR_BG);
    d.setCursor(12, 12);
    d.print("設定");

    String volumeLabel = "音量: " + String(currentVolume);
    int volumeLabelWidth = d.textWidth(volumeLabel);
    d.setCursor((w - volumeLabelWidth) / 2, layout.volumeUpButton.y - 24);
    d.print(volumeLabel);

    const SettingsRect& vd = layout.volumeDownButton;
    d.fillRoundRect(vd.x, vd.y, vd.w, vd.h, vd.h / 4, COLOR_ACCENT_DIM);
    d.setTextColor(COLOR_TEXT, COLOR_ACCENT_DIM);
    d.setCursor(vd.x + vd.w / 2 - 6, vd.y + vd.h / 2 - 8);
    d.print("-");

    const SettingsRect& vu = layout.volumeUpButton;
    d.fillRoundRect(vu.x, vu.y, vu.w, vu.h, vu.h / 4, COLOR_ACCENT_DIM);
    d.setTextColor(COLOR_TEXT, COLOR_ACCENT_DIM);
    d.setCursor(vu.x + vu.w / 2 - 6, vu.y + vu.h / 2 - 8);
    d.print("+");

    const SettingsRect& ws = layout.wifiSetupButton;
    d.fillRoundRect(ws.x, ws.y, ws.w, ws.h, ws.h / 6, COLOR_ACCENT_DIM);
    String wifiLabel = "WiFi / API キー設定";
    int wifiLabelWidth = d.textWidth(wifiLabel);
    d.setTextColor(COLOR_TEXT, COLOR_ACCENT_DIM);
    d.setCursor(ws.x + (ws.w - wifiLabelWidth) / 2, ws.y + ws.h / 2 - 8);
    d.print(wifiLabel);

    const SettingsRect& bb = layout.backButton;
    d.fillRoundRect(bb.x, bb.y, bb.w, bb.h, bb.h / 6, COLOR_ACCENT);
    String backLabel = "もどる";
    int backLabelWidth = d.textWidth(backLabel);
    d.setTextColor(COLOR_BG, COLOR_ACCENT);
    d.setCursor(bb.x + (bb.w - backLabelWidth) / 2, bb.y + bb.h / 2 - 8);
    d.print(backLabel);

    d.endWrite();
}

bool connectWifi(const WifiCredential& cred) {
    String ssid = cred.ssid.c_str();
    drawLines({"WiFi に接続中...", "SSID: " + ssid});
    Serial.printf("[wifi] connecting to %s\n", cred.ssid.c_str());
    WiFi.begin(cred.ssid.c_str(), cred.pass.c_str());
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

// 登録済み WiFi をスキャン結果と突き合わせ、RSSI 降順の候補に順に接続を試す。全滅なら false
bool connectToBestWifi(const std::vector<WifiCredential>& wifiList) {
    WiFi.mode(WIFI_STA);
    drawLines({"周辺 WiFi をスキャン中..."});
    int n = WiFi.scanNetworks();
    std::vector<WifiScanEntry> scanResults;
    for (int i = 0; i < n; ++i) {
        scanResults.push_back({WiFi.SSID(i).c_str(), WiFi.RSSI(i)});
    }
    std::vector<WifiCandidate> candidates = selectWifiCandidates(wifiList, scanResults);
    for (const auto& candidate : candidates) {
        if (connectWifi(candidate.credential)) return true;
    }
    return false;
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

// STA 常設(issue 00010): Web サーバは setup() で起動済みのため、ここでは URL 案内を表示するだけ
void enterStaUrlGuide() {
    mode = Mode::SETTINGS_URL_GUIDE;
    drawUrlGuideScreen(staPortalInfo);
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

// 状態ラベル・累計金額・エラーメッセージ(画面上部)のみを再描画する
void drawStatusArea() {
    auto& d = M5.Display;
    d.startWrite();
    d.fillRect(0, 0, d.width(), TRANSCRIPT_TOP_Y, TFT_BLACK);
    d.setFont(&fonts::efontJA_16);
    d.setTextColor(TFT_WHITE, TFT_BLACK);
    String statusLine = realtimeStateLabel(realtimeCurrentState()) + "  " +
                         usage_cost::formatJpyLabel(sessionCostJpy).c_str();
    d.setCursor(12, 12);
    d.print(statusLine);
    if (errorMessage.length() > 0) {
        d.setCursor(12, 34);
        d.print(errorMessage);
    }
    d.endWrite();
}

// 会話テキスト(画面下部)のみを再描画する。delta 到着ごとに呼ばれるためこの領域だけ更新し、
// 状態エリアの再描画・全画面クリアはしない(チラつき抑制)
void drawTranscriptArea() {
    auto& d = M5.Display;
    int h = d.height();
    d.startWrite();
    d.fillRect(0, TRANSCRIPT_TOP_Y, d.width(), h - TRANSCRIPT_TOP_Y, TFT_BLACK);
    d.setFont(&fonts::efontJA_16);
    d.setTextColor(TFT_WHITE, TFT_BLACK);
    int maxLines = std::max(1, (h - TRANSCRIPT_TOP_Y) / TRANSCRIPT_LINE_HEIGHT);
    auto lines = transcript_view::renderLines(transcriptHistory, TRANSCRIPT_WRAP_WIDTH, maxLines);
    int y = TRANSCRIPT_TOP_Y;
    for (const auto& line : lines) {
        d.setCursor(12, y);
        d.print(String(line.c_str()));
        y += TRANSCRIPT_LINE_HEIGHT;
    }
    d.endWrite();
}

void drawRealtimeScreen() {
    drawStatusArea();
    drawTranscriptArea();
}

// Idle はホーム画面、それ以外(接続中・対話中・エラー)は状態 + 会話テキストの画面
void drawForCurrentState() {
    if (realtimeCurrentState() == RealtimeState::Idle) {
        drawHomeScreenNow();
    } else {
        drawRealtimeScreen();
    }
}

// 設定画面に入る。音量変更の確認音を鳴らせるよう Speaker を起動しておく(Mic とは排他)
void enterSettings() {
    mode = Mode::SETTINGS;
    M5.Speaker.begin();
    M5.Speaker.setVolume(currentVolume);
    drawSettingsScreen();
}

void exitSettingsToHome() {
    M5.Speaker.end();
    mode = Mode::CONNECTED;
    drawForCurrentState();
}

// 音量を即時反映(表示・NVS 保存・確認音)し、対話再開時の再生音量にも反映する
void applyVolumeChange(int newVolume) {
    currentVolume = static_cast<uint8_t>(newVolume);
    M5.Speaker.setVolume(currentVolume);
    if (!configSaveVolume(currentVolume)) {
        Serial.println("[settings] volume save failed, will not persist across restart");
    }
    realtimeSetSpeakerVolume(currentVolume);
    M5.Speaker.tone(880, 120);
    drawSettingsScreen();
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
        drawStatusArea();
    };
    cb.onTranscriptUpdated = [](transcript_view::Speaker speaker, const String& text, bool finalized) {
        if (speaker == transcript_view::Speaker::User) {
            transcriptHistory = transcript_view::appendUserUtterance(transcriptHistory, text.c_str());
        } else {
            if (text.length() > 0) {
                transcriptHistory = transcript_view::appendAssistantDelta(transcriptHistory, text.c_str());
            }
            if (finalized) {
                transcriptHistory = transcript_view::finalizeAssistant(transcriptHistory);
            }
        }
        drawTranscriptArea();
    };
    return cb;
}

// タッチマーク: タップ位置に薄い円を描き、一定時間後に画面を描き直して消す。
// 各画面は全体再描画の関数を持っているため、復元はそれを呼ぶだけでよい
uint32_t touchMarkShownAtMs = 0;

void redrawCurrentScreen() {
    switch (mode) {
        case Mode::CONNECTED: drawForCurrentState(); break;
        case Mode::SETTINGS: drawSettingsScreen(); break;
        case Mode::SETTINGS_URL_GUIDE: drawUrlGuideScreen(staPortalInfo); break;
        case Mode::PORTAL: break;  // ポータル画面はタッチ操作がないためマークを出さない
    }
}

void showTouchMark(int x, int y) {
    if (mode == Mode::PORTAL) return;
    M5.Display.fillCircle(x, y, TOUCH_MARK_RADIUS, COLOR_TOUCH_MARK);
    touchMarkShownAtMs = millis();
}

void clearTouchMarkIfExpired() {
    if (touchMarkShownAtMs == 0) return;
    if (millis() - touchMarkShownAtMs < TOUCH_MARK_DURATION_MS) return;
    touchMarkShownAtMs = 0;
    redrawCurrentScreen();
}

void toggleRealtimeConnection() {
    RealtimeState state = realtimeCurrentState();
    if (state == RealtimeState::Idle || state == RealtimeState::ErrorState) {
        Serial.printf("[realtime] free heap=%u free psram=%u\n", ESP.getFreeHeap(),
                     ESP.getFreePsram());
        transcriptHistory.clear();
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
    currentVolume = config.volume;
    realtimeSetSpeakerVolume(currentVolume);
    if (!config.hasWifi()) {
        startPortal("未設定です");
        return;
    }
    if (!connectToBestWifi(config.wifiList)) {
        startPortal("WiFi 接続に失敗しました");
        return;
    }
    configTime(NTP_GMT_OFFSET_SEC, 0, NTP_SERVER_PRIMARY, NTP_SERVER_FALLBACK);
    mode = Mode::CONNECTED;
    storedApiKey = config.apiKey;
    // STA 常設(issue 00010): 設定画面を開かなくても Web サーバは起動時から待ち受ける
    staPortalInfo = portalStartSta();
    Serial.printf("[portal] sta web server started url=%s\n", staPortalInfo.url.c_str());
    drawForCurrentState();
}

void loop() {
    M5.update();
    auto touch = M5.Touch.getDetail();
    bool tapped = touch.wasPressed();
    if (tapped) {
        // 反応が悪い等の調査用に、タップの生座標と画面モードをログに残す
        Serial.printf("[touch] x=%d y=%d mode=%d\n", touch.x, touch.y, static_cast<int>(mode));
    }
    if (mode == Mode::PORTAL) {
        updatePortalScreen(portalStationCount());
        if (portalLoop()) {
            Serial.println("[portal] config saved, restarting");
            ESP.restart();
        }
    } else if (portalLoop()) {
        // STA 常設の Web サーバは画面状態に関わらず処理し続ける(QR 画面を閉じても保存できる)
        Serial.println("[portal] config saved, restarting");
        ESP.restart();
    }
    // モード分岐は else if で連鎖させる: タップ処理でモードが変わっても、同じループ内で
    // 遷移先ブロックが同一タップ(wasPressed は次の M5.update() まで立ちっぱなし)を
    // 二重処理しないようにするため(例: WiFi 設定 → URL QR 画面が開いた瞬間に閉じる)
    if (mode == Mode::CONNECTED) {
        realtimeLoop();
        RealtimeState state = realtimeCurrentState();
        if (state == RealtimeState::Idle) {
            updateHomeScreenPeriodic();
            if (tapped) {
                switch (homeScreenHitTest(M5.Display.width(), M5.Display.height(), touch.x, touch.y)) {
                    case HomeTap::Talk: toggleRealtimeConnection(); break;
                    case HomeTap::Settings: enterSettings(); break;
                    case HomeTap::None: break;
                }
            }
        } else if (tapped) {
            toggleRealtimeConnection();
        }
    } else if (mode == Mode::SETTINGS) {
        if (tapped) {
            switch (settingsScreenHitTest(M5.Display.width(), M5.Display.height(), touch.x, touch.y)) {
                case SettingsTap::VolumeUp: applyVolumeChange(settingsVolumeIncrease(currentVolume)); break;
                case SettingsTap::VolumeDown: applyVolumeChange(settingsVolumeDecrease(currentVolume)); break;
                case SettingsTap::WifiSetup:
                    if (settingsEntryAction(WiFi.status() == WL_CONNECTED) ==
                        SettingsEntryAction::ShowStaUrlGuide) {
                        enterStaUrlGuide();
                    } else {
                        M5.Speaker.end();
                        startPortal("設定");
                    }
                    break;
                case SettingsTap::Back: exitSettingsToHome(); break;
                case SettingsTap::None: break;
            }
        }
    } else if (mode == Mode::SETTINGS_URL_GUIDE) {
        if (tapped) {
            mode = Mode::SETTINGS;
            drawSettingsScreen();
        }
    }
    // 遷移処理の後に描く: 遷移先の全画面再描画でマークが消されず、必ず一瞬見えるようにする
    if (tapped) showTouchMark(touch.x, touch.y);
    clearTouchMarkIfExpired();
    delay(10);
}
