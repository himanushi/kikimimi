// セットアップポータル入力(SSID / パスワード / OpenAI API キー)の検証。
// デバイス非依存の純関数(native 環境でテストする)
#pragma once

#include <string>

struct SetupConfig {
    std::string ssid;
    std::string pass;    // オープンネットワークは空文字
    std::string apiKey;
};

struct SetupValidation {
    bool ok = false;
    std::string error;   // ok == false のとき、ブラウザに表示するメッセージ
    SetupConfig config;  // ok == true のとき、トリム済みの保存用値
};

SetupValidation validateSetupInput(const std::string& ssid, const std::string& pass,
                                   const std::string& apiKey);
