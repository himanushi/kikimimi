// 発話区間検出(エネルギーベース VAD)。デバイス非依存の純関数(native 環境でテストする)。
// 録音チャンクごとのピーク振幅から、発話開始(短いノイズでは開始しない)・
// 発話終了(ハングオーバー後の無音継続)を検知する
#pragma once
#include <cstdint>

namespace vad {

struct Config {
    int16_t startThreshold;  // この振幅(絶対値ピーク)以上を「音あり」とみなす
    uint32_t minSpeechMs;    // 音ありが連続でこの時間続いてはじめて発話開始と確定する
    uint32_t hangoverMs;     // 発話確定後、この時間連続で無音が続いたら発話終了と判定する
};

enum class Phase {
    WaitingForSpeech,  // 発話開始を待っている(無音、または未確定の音あり)
    Speaking,          // 発話開始が確定し、継続中
    Ended,             // 発話終了を検知した。以後 processChunk を呼んでも状態は変わらない
};

struct State {
    Phase phase = Phase::WaitingForSpeech;
    uint32_t aboveThresholdMs = 0;  // WaitingForSpeech 中、連続して音ありだった時間
    uint32_t silenceMs = 0;         // Speaking 中、連続して無音だった時間
};

// 1 チャンク分(chunkMs 幅)のピーク振幅を処理して状態を更新する
State processChunk(State state, int16_t peakAmplitude, uint32_t chunkMs, const Config& config);

}  // namespace vad
