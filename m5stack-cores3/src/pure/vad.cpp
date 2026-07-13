#include "vad.h"

namespace vad {

State processChunk(State state, int16_t peakAmplitude, uint32_t chunkMs, const Config& config) {
    if (state.phase == Phase::Ended) return state;

    bool above = peakAmplitude >= config.startThreshold;

    if (state.phase == Phase::WaitingForSpeech) {
        if (above) {
            state.aboveThresholdMs += chunkMs;
            if (state.aboveThresholdMs >= config.minSpeechMs) {
                state.phase = Phase::Speaking;
                state.silenceMs = 0;
            }
        } else {
            state.aboveThresholdMs = 0;  // 音が途切れたらリセット(短いノイズでは開始しない)
        }
        return state;
    }

    // Speaking
    if (above) {
        state.silenceMs = 0;
    } else {
        state.silenceMs += chunkMs;
        if (state.silenceMs >= config.hangoverMs) {
            state.phase = Phase::Ended;
        }
    }
    return state;
}

}  // namespace vad
