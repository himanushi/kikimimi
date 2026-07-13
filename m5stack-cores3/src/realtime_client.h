// OpenAI Realtime API(wss://api.openai.com/v1/realtime)への WebSocket 接続。
// 音声はまだ扱わない(00003)。接続・session.update・テキスト往復の確認までが範囲
#pragma once

#include <Arduino.h>
#include <functional>

enum class RealtimeState {
    Idle,          // 未接続(タップ待ち)
    Connecting,    // WebSocket 接続中〜session.updated 待ち
    Established,   // session.updated 受信済み。応答テキストを送受信できる
    ErrorState,    // 接続失敗・API エラー(タップで Idle に戻り再試行できる)
};

// state 遷移・応答テキストの断片・エラーメッセージを呼び出し側(画面表示)へ通知する
struct RealtimeCallbacks {
    std::function<void(RealtimeState)> onStateChanged;
    std::function<void(const String& delta)> onResponseTextDelta;
    std::function<void(const String& message)> onError;
};

// WebSocket を開始する。apiKey が空なら接続を試みず ErrorState 通知のみ行う
void realtimeConnect(const String& apiKey, const String& instructions, RealtimeCallbacks callbacks);

// 接続中なら切断して Idle に戻す(再タップでの終了用)
void realtimeDisconnect();

// loop() から毎回呼ぶ
void realtimeLoop();

RealtimeState realtimeCurrentState();

// Established 状態でユーザーメッセージを送り、応答生成を要求する
void realtimeSendUserMessage(const String& text);
