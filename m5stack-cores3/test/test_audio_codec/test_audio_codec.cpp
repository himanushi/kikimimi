// PCM16 音声チャンク ⇔ base64 変換(issue 00003 受け入れ基準)のテスト。
// input_audio_buffer.append の送信・response.output_audio.delta の受信で使う土台
#include <unity.h>

#include "pure/audio_codec.h"

using audio_codec::decodeBase64;
using audio_codec::encodeBase64;

void setUp() {}
void tearDown() {}

static void test_encode_known_bytes_matches_known_base64() {
    // "Man" -> "TWFu" は base64 の定番テストベクタ
    const uint8_t data[] = {'M', 'a', 'n'};
    TEST_ASSERT_EQUAL_STRING("TWFu", encodeBase64(data, sizeof(data)).c_str());
}

static void test_encode_applies_padding() {
    // "M" -> "TQ==" (1 byte 入力は 2 個のパディング)
    const uint8_t data[] = {'M'};
    TEST_ASSERT_EQUAL_STRING("TQ==", encodeBase64(data, sizeof(data)).c_str());
}

static void test_encode_empty_is_empty() {
    TEST_ASSERT_EQUAL_STRING("", encodeBase64(nullptr, 0).c_str());
}

static void test_decode_known_base64_matches_known_bytes() {
    auto bytes = decodeBase64("TWFu");
    TEST_ASSERT_EQUAL_UINT32(3, bytes.size());
    TEST_ASSERT_EQUAL_UINT8('M', bytes[0]);
    TEST_ASSERT_EQUAL_UINT8('a', bytes[1]);
    TEST_ASSERT_EQUAL_UINT8('n', bytes[2]);
}

static void test_decode_with_padding() {
    auto bytes = decodeBase64("TQ==");
    TEST_ASSERT_EQUAL_UINT32(1, bytes.size());
    TEST_ASSERT_EQUAL_UINT8('M', bytes[0]);
}

static void test_round_trip_pcm16_samples() {
    // 実際の用途に合わせ、PCM16 サンプル列(100ms/24kHz 相当の一部)を往復させる
    int16_t samples[] = {0, 32767, -32768, 12345, -12345};
    auto* bytes = reinterpret_cast<uint8_t*>(samples);
    size_t len = sizeof(samples);
    std::string encoded = encodeBase64(bytes, len);
    auto decoded = decodeBase64(encoded);
    TEST_ASSERT_EQUAL_UINT32(len, decoded.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(bytes, decoded.data(), len);
}

static void test_decode_invalid_characters_returns_empty() {
    auto bytes = decodeBase64("not@@valid!!base64");
    TEST_ASSERT_TRUE(bytes.empty());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_encode_known_bytes_matches_known_base64);
    RUN_TEST(test_encode_applies_padding);
    RUN_TEST(test_encode_empty_is_empty);
    RUN_TEST(test_decode_known_base64_matches_known_bytes);
    RUN_TEST(test_decode_with_padding);
    RUN_TEST(test_round_trip_pcm16_samples);
    RUN_TEST(test_decode_invalid_characters_returns_empty);
    return UNITY_END();
}
