// STT(音声認識)→ LLM(chat/completions)→ TTS(音声合成)の直列パイプラインでの音声対話。
// マイク録音 → OpenAI へ順に HTTP POST → 応答音声をスピーカーで一括再生
#pragma once

#include <Arduino.h>
#include <functional>

#include "pure/transcript_view.h"

enum class VoicePipelineState {
    Idle,          // 未開始(タップ待ち)
    Listening,     // マイクで録音中(聞いている)
    Thinking,      // STT → LLM → TTS のリクエスト中(考えている)
    Speaking,      // 応答音声を再生中(話している)
    ErrorState,    // 通信エラー・API エラー(タップで Idle に戻り再試行できる)
};

// state 遷移・エラーメッセージ・セッション累計金額を呼び出し側(画面表示)へ通知する
struct VoicePipelineCallbacks {
    std::function<void(VoicePipelineState)> onStateChanged;
    std::function<void(const String& message)> onError;
    std::function<void(double sessionCostJpy)> onCostUpdated;
    // ユーザー・アシスタントの発話は STT/chat 応答が確定してから一度に通知する(finalized は常に true)
    std::function<void(transcript_view::Speaker speaker, const String& text, bool finalized)> onTranscriptUpdated;
};

// 応答音声の再生音量(0..255)。次回の Speaker.begin() 時から反映する
void voicePipelineSetSpeakerVolume(uint8_t volume);

// 録音・応答バッファ(PSRAM)を確保し、対話を開始する(タップ開始時に呼ぶ)。
// apiKey が空なら開始を試みず ErrorState 通知のみ行う
void voicePipelineStart(const String& apiKey, const String& instructions, VoicePipelineCallbacks callbacks);

// 対話を中断して Idle に戻す(再タップでの終了用)
void voicePipelineStop();

// loop() から毎回呼ぶ。Listening 中はここで録音・VAD 判定を行い、発話終了を検知すると
// STT → chat → TTS を順に呼び出す(この呼び出し中は loop() をブロックする)
void voicePipelineLoop();

VoicePipelineState voicePipelineCurrentState();
