// gpt-realtime-mini のトークン使用量 → 金額(円)換算。デバイス非依存の純関数(native 環境でテストする)
#pragma once

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

// 1 回の response.done の usage を円に換算する。cached トークンは入力トークン数の
// 内数のため、非 cached 分と cached 分をそれぞれの単価で計算して合算する
double calculateCostJpy(const realtime_protocol::UsageTokens& usage);

// 円額を「¥12.3」形式(小数第1位)に整形する
std::string formatJpyLabel(double jpy);

}  // namespace usage_cost
