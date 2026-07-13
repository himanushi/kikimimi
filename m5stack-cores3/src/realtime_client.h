// OpenAI Realtime API(wss://api.openai.com/v1/realtime)への WebSocket 接続と音声対話。
// マイク録音 → input_audio_buffer.append 送信、response.output_audio.delta → スピーカー再生
#pragma once

#include <Arduino.h>
#include <functional>

#include "pure/transcript_view.h"

enum class RealtimeState {
    Idle,          // 未接続(タップ待ち)
    Connecting,    // WebSocket 接続中〜session.updated 待ち
    Listening,     // マイクで録音・送信中(聞いている)
    Thinking,      // 発話終了を検知し、応答音声を待っている(考えている)
    Speaking,      // 応答音声を再生中(話している)
    ErrorState,    // 接続失敗・API エラー(タップで Idle に戻り再試行できる)
};

// state 遷移・エラーメッセージ・セッション累計金額を呼び出し側(画面表示)へ通知する
struct RealtimeCallbacks {
    std::function<void(RealtimeState)> onStateChanged;
    std::function<void(const String& message)> onError;
    std::function<void(double sessionCostJpy)> onCostUpdated;
    // 発話の書き起こし更新。finalized=false はアシスタント delta の逐次追記、true は発話確定
    // (ユーザー発話は常に確定済みの全文で一度だけ、アシスタント発話は確定時に空文字で通知する)
    std::function<void(transcript_view::Speaker speaker, const String& text, bool finalized)> onTranscriptUpdated;
};

// 応答音声の再生音量(0..255)。次回の Speaker.begin() 時から反映する
void realtimeSetSpeakerVolume(uint8_t volume);

// WebSocket を開始する。apiKey が空なら接続を試みず ErrorState 通知のみ行う
void realtimeConnect(const String& apiKey, const String& instructions, RealtimeCallbacks callbacks);

// 接続中なら切断して Idle に戻す(再タップでの終了用)
void realtimeDisconnect();

// loop() から毎回呼ぶ。Listening 中はここでマイクの録音・送信も行う(ブロッキング)
void realtimeLoop();

RealtimeState realtimeCurrentState();
