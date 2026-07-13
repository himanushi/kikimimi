#include "usage_cost.h"

#include <cstdio>

namespace usage_cost {

namespace {

double tokensToUsd(long tokens, double usdPerMillion) {
    return static_cast<double>(tokens) / 1000000.0 * usdPerMillion;
}

}  // namespace

double calculateCostJpy(const realtime_protocol::UsageTokens& usage) {
    // cached は input トークン数の内数のため、非 cached 分だけを通常単価で計算する
    long nonCachedInputText = usage.inputTextTokens - usage.cachedTextTokens;
    long nonCachedInputAudio = usage.inputAudioTokens - usage.cachedAudioTokens;

    double usd = tokensToUsd(nonCachedInputText, TEXT_INPUT_USD_PER_MILLION) +
                 tokensToUsd(nonCachedInputAudio, AUDIO_INPUT_USD_PER_MILLION) +
                 tokensToUsd(usage.cachedTextTokens, CACHED_INPUT_USD_PER_MILLION) +
                 tokensToUsd(usage.cachedAudioTokens, CACHED_INPUT_USD_PER_MILLION) +
                 tokensToUsd(usage.outputTextTokens, TEXT_OUTPUT_USD_PER_MILLION) +
                 tokensToUsd(usage.outputAudioTokens, AUDIO_OUTPUT_USD_PER_MILLION);

    return usd * USD_TO_JPY;
}

std::string formatJpyLabel(double jpy) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "\xC2\xA5%.1f", jpy);  // ¥ (U+00A5) の UTF-8 表現
    return std::string(buf);
}

}  // namespace usage_cost
