// kikimimi CoreS3 ファーム: 起動 → 保存済み WiFi へ接続、未設定・接続失敗ならセットアップポータル
#include <M5Unified.h>
#include <WiFi.h>

#include "config_store.h"
#include "portal.h"

namespace {

enum class Mode { PORTAL, CONNECTED };
Mode mode = Mode::PORTAL;

constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;

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

void startPortal(const String& reason) {
    PortalInfo info = portalStart();
    // AP パスワードはシリアルに出さない(画面にのみ表示)
    Serial.printf("[portal] started ap=%s url=%s\n", info.apName.c_str(), info.url.c_str());
    drawLines({reason, "スマホで WiFi に接続:", "  " + info.apName + " / " + info.apPass,
               "ブラウザで開く:", "  " + info.url});
    mode = Mode::PORTAL;
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
    drawLines({"接続しました", "SSID: " + config.ssid, "IP: " + WiFi.localIP().toString(),
               "", "タップで対話(未実装)"});
}

void loop() {
    M5.update();
    if (mode == Mode::PORTAL && portalLoop()) {
        Serial.println("[portal] config saved, restarting");
        ESP.restart();
    }
    delay(10);
}
