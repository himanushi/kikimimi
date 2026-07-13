#include "setup_config.h"

namespace {

std::string trimmed(const std::string& s) {
    const char* ws = " \t\r\n";
    size_t begin = s.find_first_not_of(ws);
    if (begin == std::string::npos) return "";
    size_t end = s.find_last_not_of(ws);
    return s.substr(begin, end - begin + 1);
}

SetupValidation fail(const std::string& error) {
    SetupValidation r;
    r.ok = false;
    r.error = error;
    return r;
}

}  // namespace

SetupValidation validateSetupInput(const std::string& ssid, const std::string& pass,
                                   const std::string& apiKey) {
    SetupValidation r;
    r.config.ssid = trimmed(ssid);
    r.config.pass = pass;  // パスワードは先頭・末尾の空白も有効な文字なのでトリムしない
    r.config.apiKey = trimmed(apiKey);

    if (r.config.ssid.empty()) return fail("SSID を入力してください");
    if (r.config.ssid.size() > 32) return fail("SSID が長すぎます(32 バイト以内)");

    // WPA2 のパスフレーズは 8〜63 文字。空はオープンネットワークとして許可
    if (!r.config.pass.empty() && (r.config.pass.size() < 8 || r.config.pass.size() > 63)) {
        return fail("WiFi パスワードは 8〜63 文字です(オープンネットワークは空欄)");
    }

    if (r.config.apiKey.empty()) return fail("API キーを入力してください");
    if (r.config.apiKey.rfind("sk-", 0) != 0) return fail("API キーは sk- で始まる形式です");
    if (r.config.apiKey.size() < 20 || r.config.apiKey.size() > 512) {
        return fail("API キーの長さが正しくありません");
    }
    for (char c : r.config.apiKey) {
        if (c <= 0x20 || c > 0x7E) return fail("API キーに使えない文字が含まれています");
    }

    r.ok = true;
    return r;
}
