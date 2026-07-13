#include "portal.h"

#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>

#include "config_store.h"
#include "pure/setup_config.h"

namespace {

constexpr const char* AP_NAME = "kikimimi-setup";

DNSServer dnsServer;
WebServer webServer(80);
bool saved = false;
uint32_t savedAtMs = 0;

String htmlEscape(const String& s) {
    String out;
    out.reserve(s.length());
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default: out += c;
        }
    }
    return out;
}

String formPage(const String& error, const String& ssid) {
    String page =
        "<!DOCTYPE html><html lang=\"ja\"><head><meta charset=\"utf-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>kikimimi setup</title></head><body>"
        "<h1>kikimimi 初期設定</h1>";
    if (error.length()) page += "<p style=\"color:red\">" + htmlEscape(error) + "</p>";
    page +=
        "<form method=\"POST\" action=\"/save\">"
        "<p><label>WiFi SSID<br><input name=\"ssid\" value=\"" + htmlEscape(ssid) + "\"></label></p>"
        "<p><label>WiFi パスワード<br><input name=\"pass\" type=\"password\"></label></p>"
        "<p><label>OpenAI API キー<br><input name=\"apikey\" autocomplete=\"off\"></label></p>"
        "<p><button type=\"submit\">保存して再起動</button></p>"
        "</form></body></html>";
    return page;
}

void handleRoot() { webServer.send(200, "text/html", formPage("", "")); }

void handleSave() {
    SetupValidation r = validateSetupInput(webServer.arg("ssid").c_str(),
                                           webServer.arg("pass").c_str(),
                                           webServer.arg("apikey").c_str());
    if (!r.ok) {
        webServer.send(200, "text/html", formPage(r.error.c_str(), webServer.arg("ssid")));
        return;
    }
    StoredConfig config;
    config.ssid = r.config.ssid.c_str();
    config.pass = r.config.pass.c_str();
    config.apiKey = r.config.apiKey.c_str();
    if (!configSave(config)) {
        webServer.send(500, "text/html",
                       "<!DOCTYPE html><html lang=\"ja\"><head><meta charset=\"utf-8\"></head><body>"
                       "<h1>保存に失敗しました</h1><p>本体の保存領域に書き込めませんでした。"
                       "電源を入れ直してやり直してください。</p></body></html>");
        return;
    }
    webServer.send(200, "text/html",
                   "<!DOCTYPE html><html lang=\"ja\"><head><meta charset=\"utf-8\"></head><body>"
                   "<h1>保存しました</h1><p>再起動して WiFi に接続します。</p></body></html>");
    saved = true;
    savedAtMs = millis();
}

// captive portal 検出(iOS の /hotspot-detect.html 等)を含む未知の URL は設定画面へ誘導する
void handleNotFound() {
    webServer.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/");
    webServer.send(302, "text/plain", "");
}

}  // namespace

// オープン AP だとフォーム送信(API キー)が無線区間で平文になるため WPA2 にする。
// パスワードは起動ごとの乱数を本体画面にだけ表示する(固定値だと public repo に載ってしまう)
static String generateApPass() {
    String pass;
    for (int i = 0; i < 8; ++i) pass += char('0' + esp_random() % 10);
    return pass;
}

PortalInfo portalStart() {
    String apPass = generateApPass();
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_NAME, apPass.c_str());
    dnsServer.start(53, "*", WiFi.softAPIP());
    webServer.on("/", HTTP_GET, handleRoot);
    webServer.on("/save", HTTP_POST, handleSave);
    webServer.onNotFound(handleNotFound);
    webServer.begin();
    return {AP_NAME, apPass, String("http://") + WiFi.softAPIP().toString() + "/"};
}

bool portalLoop() {
    dnsServer.processNextRequest();
    webServer.handleClient();
    // 保存応答がブラウザへ届く猶予をとってから再起動を要求する
    return saved && (millis() - savedAtMs > 2000);
}

int portalStationCount() { return WiFi.softAPgetStationNum(); }
