// gpt-realtime のトークン使用量 → 金額(円)換算。デバイス非依存の純関数(native 環境でテストする)
#pragma once

#include <string>

#include "realtime_protocol.h"

namespace usage_cost {

// 為替は 160 円/USD の固定定数(issue 00007 plan: 動的取得はやらない)
constexpr double USD_TO_JPY = 160.0;

// gpt-realtime の公式単価(1M トークンあたり USD)。
// 出典: https://developers.openai.com/api/docs/models/gpt-realtime (2026-07 確認)
constexpr double TEXT_INPUT_USD_PER_MILLION = 4.0;
constexpr double TEXT_OUTPUT_USD_PER_MILLION = 16.0;
constexpr double AUDIO_INPUT_USD_PER_MILLION = 32.0;
constexpr double AUDIO_OUTPUT_USD_PER_MILLION = 64.0;
// cached input はテキスト・音声とも同一単価
constexpr double CACHED_INPUT_USD_PER_MILLION = 0.40;

// 1 回の response.done の usage を円に換算する。cached トークンは入力トークン数の
// 内数のため、非 cached 分と cached 分をそれぞれの単価で計算して合算する
double calculateCostJpy(const realtime_protocol::UsageTokens& usage);

// 円額を「¥12.3」形式(小数第1位)に整形する
std::string formatJpyLabel(double jpy);

}  // namespace usage_cost
