// エネルギーベース VAD(issue 00014 受け入れ基準)のテスト。
// 仕様: 音あり(閾値以上)が minSpeechMs 続いてはじめて発話開始とみなす(短いノイズでは開始しない)。
// 発話開始後、無音が hangoverMs 続いたら発話終了とみなす(ハングオーバー)。
#include <unity.h>

#include "pure/vad.h"

using vad::Config;
using vad::Phase;
using vad::processChunk;
using vad::State;

void setUp() {}
void tearDown() {}

constexpr Config kConfig{/*startThreshold=*/1000, /*minSpeechMs=*/200, /*hangoverMs=*/700};
constexpr uint32_t kChunkMs = 100;

static void test_silence_only_stays_waiting_for_speech() {
    State s;
    for (int i = 0; i < 10; ++i) {
        s = processChunk(s, 0, kChunkMs, kConfig);
    }
    TEST_ASSERT_TRUE(s.phase == Phase::WaitingForSpeech);
}

static void test_speech_then_silence_ends_after_hangover() {
    State s;
    // 200ms 分の音あり → 発話開始が確定する
    s = processChunk(s, 2000, kChunkMs, kConfig);
    s = processChunk(s, 2000, kChunkMs, kConfig);
    TEST_ASSERT_TRUE(s.phase == Phase::Speaking);
    // hangoverMs(700ms) 未満の無音ではまだ終了しない
    s = processChunk(s, 0, kChunkMs, kConfig);
    s = processChunk(s, 0, kChunkMs, kConfig);
    s = processChunk(s, 0, kChunkMs, kConfig);
    s = processChunk(s, 0, kChunkMs, kConfig);
    s = processChunk(s, 0, kChunkMs, kConfig);
    s = processChunk(s, 0, kChunkMs, kConfig);
    TEST_ASSERT_TRUE(s.phase == Phase::Speaking);
    // 7 チャンク目(累計 700ms)で終了確定
    s = processChunk(s, 0, kChunkMs, kConfig);
    TEST_ASSERT_TRUE(s.phase == Phase::Ended);
}

static void test_short_noise_does_not_start_speech() {
    State s;
    // 100ms だけ音あり(minSpeechMs=200ms 未満)→ すぐ無音に戻る
    s = processChunk(s, 2000, kChunkMs, kConfig);
    s = processChunk(s, 0, kChunkMs, kConfig);
    TEST_ASSERT_TRUE(s.phase == Phase::WaitingForSpeech);
    // 音ありの連続カウントがリセットされているか(再度 100ms 音ありでも開始しない)
    s = processChunk(s, 2000, kChunkMs, kConfig);
    TEST_ASSERT_TRUE(s.phase == Phase::WaitingForSpeech);
}

static void test_hangover_resets_on_resumed_speech() {
    State s;
    s = processChunk(s, 2000, kChunkMs, kConfig);
    s = processChunk(s, 2000, kChunkMs, kConfig);
    TEST_ASSERT_TRUE(s.phase == Phase::Speaking);
    // 500ms 無音(hangover 未満)のあと発話再開 → 終了しない
    s = processChunk(s, 0, kChunkMs, kConfig);
    s = processChunk(s, 0, kChunkMs, kConfig);
    s = processChunk(s, 0, kChunkMs, kConfig);
    s = processChunk(s, 0, kChunkMs, kConfig);
    s = processChunk(s, 0, kChunkMs, kConfig);
    s = processChunk(s, 2000, kChunkMs, kConfig);
    TEST_ASSERT_TRUE(s.phase == Phase::Speaking);
    // ここから 700ms 無音を続けないと終了しない(500ms では終了しない)ことを確認
    s = processChunk(s, 0, kChunkMs, kConfig);
    s = processChunk(s, 0, kChunkMs, kConfig);
    s = processChunk(s, 0, kChunkMs, kConfig);
    s = processChunk(s, 0, kChunkMs, kConfig);
    s = processChunk(s, 0, kChunkMs, kConfig);
    TEST_ASSERT_TRUE(s.phase == Phase::Speaking);
}

static void test_ended_state_is_final() {
    State s;
    s.phase = Phase::Ended;
    s = processChunk(s, 2000, kChunkMs, kConfig);
    TEST_ASSERT_TRUE(s.phase == Phase::Ended);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_silence_only_stays_waiting_for_speech);
    RUN_TEST(test_speech_then_silence_ends_after_hangover);
    RUN_TEST(test_short_noise_does_not_start_speech);
    RUN_TEST(test_hangover_resets_on_resumed_speech);
    RUN_TEST(test_ended_state_is_final);
    return UNITY_END();
}
