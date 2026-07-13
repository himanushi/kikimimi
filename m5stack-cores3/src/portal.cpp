#include "portal.h"

#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>

#include <vector>

#include "config_store.h"
#include "pure/setup_config.h"
#include "pure/wifi_scan.h"

namespace {

constexpr const char* AP_NAME = "kikimimi-setup";
constexpr const char* MANUAL_OPTION = "";  // select の値が空 = 手入力欄を使う
// DNSServer が AP モード中ワイルドカード解決しているため、IP の代わりにこのドメインで案内できる(STA では使えない)
constexpr const char* PORTAL_DOMAIN = "kiki.mimi";

DNSServer dnsServer;
WebServer webServer(80);
bool saved = false;
uint32_t savedAtMs = 0;
bool apActive = false;  // true: AP モード(captive DNS・接続台数あり) / false: STA 常設(Web サーバのみ)

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

// 周辺 WiFi の一覧を <select> の選択肢 HTML にする。空 SSID 除外・重複除去・RSSI 降順は純関数側の責務。
// selectedSsid はバリデーションエラー再表示時に選択状態を保つため(空なら「手入力欄を使う」を選択)
String scanOptionsHtml(const String& selectedSsid) {
    int n = WiFi.scanComplete();
    std::vector<WifiScanEntry> raw;
    for (int i = 0; i < n; ++i) {
        raw.push_back({WiFi.SSID(i).c_str(), WiFi.RSSI(i)});
    }
    std::vector<WifiScanEntry> entries = dedupeSortWifiScan(raw);
    String options = String("<option value=\"") + MANUAL_OPTION + "\"" +
                     (selectedSsid.length() ? "" : " selected") + ">(手入力欄を使う)</option>";
    for (const auto& e : entries) {
        String ssid = htmlEscape(String(e.ssid.c_str()));
        options += "<option value=\"" + ssid + "\"" + (ssid == selectedSsid ? " selected" : "") +
                  ">" + ssid + " (" + String(e.rssi) + " dBm)</option>";
    }
    return options;
}

// WiFi.scanComplete() の状態に応じて select またはスキャン中メッセージを返す
String scanSectionHtml(const String& selectedSsid) {
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING) {
        return "<p>周辺 WiFi をスキャン中です。数秒後にページを再読み込みしてください。"
               "<a href=\"/\">再読み込み</a></p>";
    }
    if (n == WIFI_SCAN_FAILED) {
        WiFi.scanNetworks(true);  // 未実行または失敗: 非同期で開始し、結果は次回読み込みで表示する
        return "<p>周辺 WiFi をスキャン中です。数秒後にページを再読み込みしてください。"
               "<a href=\"/\">再読み込み</a></p>";
    }
    return "<p><label>周辺の WiFi<br><select name=\"ssid_select\">" + scanOptionsHtml(selectedSsid) +
          "</select></label> <a href=\"/rescan\">再スキャン</a></p>";
}

String formPage(const String& error, const String& ssid, const String& selectedSsid) {
    String page =
        "<!DOCTYPE html><html lang=\"ja\"><head><meta charset=\"utf-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>kikimimi setup</title></head><body>"
        "<h1>kikimimi 初期設定</h1>";
    if (error.length()) page += "<p style=\"color:red\">" + htmlEscape(error) + "</p>";
    page +=
        "<form method=\"POST\" action=\"/save\">" + scanSectionHtml(selectedSsid) +
        "<p><label>WiFi SSID 手入力(一覧にない場合)<br>"
        "<input name=\"ssid\" value=\"" + htmlEscape(ssid) + "\"></label></p>"
        "<p><label>WiFi パスワード<br><input name=\"pass\" type=\"password\"></label></p>"
        "<p><label>OpenAI API キー<br><input name=\"apikey\" autocomplete=\"off\"></label></p>"
        "<p><button type=\"submit\">保存して再起動</button></p>"
        "</form></body></html>";
    return page;
}

void handleRoot() { webServer.send(200, "text/html", formPage("", "", "")); }

void handleRescan() {
    WiFi.scanNetworks(true);
    webServer.sendHeader("Location", "/");
    webServer.send(302, "text/plain", "");
}

void handleSave() {
    // 手入力が優先: 手入力欄が空のときだけ select の選択値を使う
    String manualSsid = webServer.arg("ssid");
    String selectSsid = webServer.arg("ssid_select");
    String ssid = manualSsid.length() ? manualSsid : selectSsid;
    SetupValidation r = validateSetupInput(ssid.c_str(),
                                           webServer.arg("pass").c_str(),
                                           webServer.arg("apikey").c_str());
    if (!r.ok) {
        webServer.send(200, "text/html", formPage(r.error.c_str(), manualSsid, selectSsid));
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

// captive portal 検出(iOS の /hotspot-detect.html 等)を含む未知の URL は設定画面へ誘導する。
// kiki.mimi は AP モードの DNSServer ワイルドカードでのみ解決できるため、STA では使わない
void handleNotFound() {
    webServer.sendHeader("Location", apActive ? String("http://") + PORTAL_DOMAIN + "/" : "/");
    webServer.send(302, "text/plain", "");
}

// AP・STA 共通のフォーム・保存・再スキャンハンドラを登録する
void registerFormHandlers() {
    webServer.on("/", HTTP_GET, handleRoot);
    webServer.on("/rescan", HTTP_GET, handleRescan);
    webServer.on("/save", HTTP_POST, handleSave);
    webServer.onNotFound(handleNotFound);
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
    apActive = true;
    String apPass = generateApPass();
    // AP_STA: 同期スキャン(WIFI_STA 単体)は SoftAP を止めてしまうため、AP を維持したまま STA 側でスキャンする
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_NAME, apPass.c_str());
    WiFi.scanNetworks(true);  // 非同期スキャン開始。結果は handleRoot が WiFi.scanComplete() で取得する
    dnsServer.start(53, "*", WiFi.softAPIP());
    registerFormHandlers();
    webServer.begin();
    return {AP_NAME, apPass, String("http://") + PORTAL_DOMAIN + "/"};
}

PortalInfo portalStartSta() {
    apActive = false;
    registerFormHandlers();
    webServer.begin();
    return {"", "", String("http://") + WiFi.localIP().toString() + "/"};
}

bool portalLoop() {
    if (apActive) dnsServer.processNextRequest();
    webServer.handleClient();
    // 保存応答がブラウザへ届く猶予をとってから再起動を要求する
    return saved && (millis() - savedAtMs > 2000);
}

int portalStationCount() { return apActive ? WiFi.softAPgetStationNum() : 0; }
