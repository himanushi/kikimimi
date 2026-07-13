// gpt-realtime-mini のトークン使用量 → 金額(円)換算。デバイス非依存の純関数(native 環境でテストする)
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "realtime_protocol.h"

namespace usage_cost {

// 為替は 160 円/USD の固定定数(issue 00007 plan: 動的取得はやらない)
constexpr double USD_TO_JPY = 160.0;

// gpt-realtime-mini の公式単価(1M トークンあたり USD)。
// 出典: https://developers.openai.com/api/docs/models/gpt-realtime-mini (2026-07 確認)
constexpr double TEXT_INPUT_USD_PER_MILLION = 0.60;
constexpr double TEXT_OUTPUT_USD_PER_MILLION = 2.40;
constexpr double AUDIO_INPUT_USD_PER_MILLION = 10.0;
constexpr double AUDIO_OUTPUT_USD_PER_MILLION = 20.0;
// cached input は mini ではテキスト・音声で単価が異なる
constexpr double CACHED_TEXT_INPUT_USD_PER_MILLION = 0.06;
constexpr double CACHED_AUDIO_INPUT_USD_PER_MILLION = 0.30;

// STT/LLM/TTS パイプライン(issue 00014)の 3 モデルの公式単価(1M トークンあたり USD)。
// 出典: https://developers.openai.com/api/docs/pricing (2026-07 確認)
constexpr double CHAT_INPUT_USD_PER_MILLION = 0.05;    // gpt-5-nano
constexpr double CHAT_OUTPUT_USD_PER_MILLION = 0.40;   // gpt-5-nano
constexpr double STT_AUDIO_INPUT_USD_PER_MILLION = 3.00;  // gpt-4o-mini-transcribe
constexpr double STT_TEXT_OUTPUT_USD_PER_MILLION = 5.00;  // gpt-4o-mini-transcribe
constexpr double TTS_TEXT_INPUT_USD_PER_MILLION = 0.60;   // gpt-4o-mini-tts
constexpr double TTS_AUDIO_OUTPUT_USD_PER_MILLION = 12.00; // gpt-4o-mini-tts
// gpt-4o-mini-tts はレスポンスに usage を返さないため音声出力トークン数は概算する。
// OpenAI の目安($0.015/分, $12/M)から逆算した 1 秒あたりの音声トークン数(概算値)
constexpr double TTS_AUDIO_TOKENS_PER_SECOND_ESTIMATE = 20.0;

// 1 回の response.done の usage を円に換算する。cached トークンは入力トークン数の
// 内数のため、非 cached 分と cached 分をそれぞれの単価で計算して合算する
double calculateCostJpy(const realtime_protocol::UsageTokens& usage);

// gpt-5-nano の chat/completions 1 回分を円換算する。cachedTokens は promptTokens の内数
double calculateChatCostJpy(long promptTokens, long completionTokens, long cachedTokens);

// gpt-4o-mini-transcribe の audio/transcriptions 1 回分を円換算する
// (usage.input_tokens は音声入力、usage.output_tokens はテキスト出力)
double calculateSttCostJpy(long inputAudioTokens, long outputTextTokens);

// gpt-4o-mini-tts の audio/speech 1 回分の概算円換算。usage が返らないため、
// 入力テキストの UTF-8 バイト数(日本語主体のため概ね 1 文字 ≒ 1 トークンとみなせる 3 バイト/トークンで近似)と、
// 受信 PCM のバイト長(サンプルレートから再生時間を逆算し、音声トークン数を概算)から算出する
double estimateTtsCostJpy(size_t inputTextUtf8Bytes, size_t pcmBytes, uint32_t sampleRate);

// 円額を「¥12.3」形式(小数第1位)に整形する
std::string formatJpyLabel(double jpy);

}  // namespace usage_cost
