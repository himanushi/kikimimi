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
                 tokensToUsd(usage.cachedTextTokens, CACHED_TEXT_INPUT_USD_PER_MILLION) +
                 tokensToUsd(usage.cachedAudioTokens, CACHED_AUDIO_INPUT_USD_PER_MILLION) +
                 tokensToUsd(usage.outputTextTokens, TEXT_OUTPUT_USD_PER_MILLION) +
                 tokensToUsd(usage.outputAudioTokens, AUDIO_OUTPUT_USD_PER_MILLION);

    return usd * USD_TO_JPY;
}

double calculateChatCostJpy(long promptTokens, long completionTokens, long cachedTokens) {
    // issue 00014 では cached を非 cached と同一単価で扱う(gpt-5-nano の cached 単価は
    // 別途確認が必要になった時点で拡張する)。cachedTokens は promptTokens の内数のため二重加算しない
    (void)cachedTokens;
    double usd = tokensToUsd(promptTokens, CHAT_INPUT_USD_PER_MILLION) +
                 tokensToUsd(completionTokens, CHAT_OUTPUT_USD_PER_MILLION);
    return usd * USD_TO_JPY;
}

double calculateSttCostJpy(long inputAudioTokens, long outputTextTokens) {
    double usd = tokensToUsd(inputAudioTokens, STT_AUDIO_INPUT_USD_PER_MILLION) +
                 tokensToUsd(outputTextTokens, STT_TEXT_OUTPUT_USD_PER_MILLION);
    return usd * USD_TO_JPY;
}

double estimateTtsCostJpy(size_t inputTextUtf8Bytes, size_t pcmBytes, uint32_t sampleRate) {
    if (inputTextUtf8Bytes == 0 && pcmBytes == 0) return 0.0;
    // 日本語主体のテキストは概ね 1 文字(UTF-8 3 byte)≒ 1 トークンとみなして近似する
    double inputTokens = static_cast<double>(inputTextUtf8Bytes) / 3.0;
    double durationSeconds = sampleRate > 0
        ? static_cast<double>(pcmBytes) / (sampleRate * 2.0 /*bytes per 16bit sample*/)
        : 0.0;
    double outputTokens = durationSeconds * TTS_AUDIO_TOKENS_PER_SECOND_ESTIMATE;

    double usd = tokensToUsd(static_cast<long>(inputTokens), TTS_TEXT_INPUT_USD_PER_MILLION) +
                 tokensToUsd(static_cast<long>(outputTokens), TTS_AUDIO_OUTPUT_USD_PER_MILLION);
    return usd * USD_TO_JPY;
}

std::string formatJpyLabel(double jpy) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "\xC2\xA5%.1f", jpy);  // ¥ (U+00A5) の UTF-8 表現
    return std::string(buf);
}

}  // namespace usage_cost
