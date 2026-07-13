// OpenAI Realtime API(wss://api.openai.com/v1/realtime)への WebSocket 接続と音声対話。
// マイク録音 → input_audio_buffer.append 送信、response.output_audio.delta → スピーカー再生
#pragma once

#include <Arduino.h>
#include <functional>

enum class RealtimeState {
    Idle,          // 未接続(タップ待ち)
    Connecting,    // WebSocket 接続中〜session.updated 待ち
    Listening,     // マイクで録音・送信中(聞いている)
    Thinking,      // 発話終了を検知し、応答音声を待っている(考えている)
    Speaking,      // 応答音声を再生中(話している)
    ErrorState,    // 接続失敗・API エラー(タップで Idle に戻り再試行できる)
};

// state 遷移・エラーメッセージを呼び出し側(画面表示)へ通知する
struct RealtimeCallbacks {
    std::function<void(RealtimeState)> onStateChanged;
    std::function<void(const String& message)> onError;
};

// WebSocket を開始する。apiKey が空なら接続を試みず ErrorState 通知のみ行う
void realtimeConnect(const String& apiKey, const String& instructions, RealtimeCallbacks callbacks);

// 接続中なら切断して Idle に戻す(再タップでの終了用)
void realtimeDisconnect();

// loop() から毎回呼ぶ。Listening 中はここでマイクの録音・送信も行う(ブロッキング)
void realtimeLoop();

RealtimeState realtimeCurrentState();
