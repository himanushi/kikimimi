// kikimimi CoreS3 ファーム: 起動 → 保存済み WiFi へ接続、未設定・接続失敗ならセットアップポータル
#include <M5Unified.h>
#include <WiFi.h>

#include "config_store.h"
#include "portal.h"
#include "pure/wifi_qr.h"
#include "realtime_client.h"

namespace {

enum class Mode { PORTAL, CONNECTED };
Mode mode = Mode::PORTAL;

constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr const char* SESSION_INSTRUCTIONS = "あなたは kikimimi という名前の相棒です。短く日本語で応答してください。";

String storedApiKey;
String errorMessage;

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

// 左: テキスト情報 / 右: QR(QR は AP 接続専用。API キーが乗る設定 URL 側は QR 化しない)
void drawPortalScreen(const String& reason, const PortalInfo& info) {
    drawLines({reason, "スマホで WiFi に接続:", "  " + info.apName,
              "  パスワード: " + info.apPass, "", "ブラウザで開く:", "  " + info.url});
    // テキスト行(x=12 起点)と QR が重ならないよう、QR は画面右端寄せで幅を絞る
    std::string qrPayload = buildWifiQrPayload(info.apName.c_str(), info.apPass.c_str());
    M5.Display.qrcode(qrPayload.c_str(), 210, 50, 100);
}

void startPortal(const String& reason) {
    PortalInfo info = portalStart();
    // AP パスワードはシリアルに出さない(画面にのみ表示)
    Serial.printf("[portal] started ap=%s url=%s\n", info.apName.c_str(), info.url.c_str());
    drawPortalScreen(reason, info);
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
    drawLines({realtimeStateLabel(realtimeCurrentState()), "", errorMessage});
}

RealtimeCallbacks buildRealtimeCallbacks() {
    RealtimeCallbacks cb;
    cb.onStateChanged = [](RealtimeState state) {
        if (state == RealtimeState::Listening) errorMessage = "";
        drawRealtimeScreen();
    };
    cb.onError = [](const String& message) {
        errorMessage = message;
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
        drawRealtimeScreen();
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
    mode = Mode::CONNECTED;
    storedApiKey = config.apiKey;
    drawRealtimeScreen();
}

void loop() {
    M5.update();
    if (mode == Mode::PORTAL && portalLoop()) {
        Serial.println("[portal] config saved, restarting");
        ESP.restart();
    }
    if (mode == Mode::CONNECTED) {
        realtimeLoop();
        if (M5.Touch.getDetail().wasPressed()) toggleRealtimeConnection();
    }
    delay(10);
}
