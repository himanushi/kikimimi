// WAV(RIFF)ヘッダ生成(issue 00014 受け入れ基準)のテスト。
// 仕様: 16bit mono の PCM に対し、サンプルレート・データ長が正しく入った 44 byte の RIFF ヘッダを書く。
#include <unity.h>
#include <string.h>

#include "pure/wav.h"

void setUp() {}
void tearDown() {}

static uint16_t readU16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

static uint32_t readU32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

static void test_header_size_is_44_bytes() {
    uint8_t buf[44];
    size_t written = wavWriteHeader(buf, 16000, 16, 1, 1000);
    TEST_ASSERT_EQUAL_UINT(44, written);
    TEST_ASSERT_EQUAL_UINT(44, WAV_HEADER_SIZE);
}

static void test_riff_wave_magic_and_chunk_ids() {
    uint8_t buf[44];
    wavWriteHeader(buf, 16000, 16, 1, 1000);
    TEST_ASSERT_EQUAL_UINT8_ARRAY("RIFF", buf, 4);
    TEST_ASSERT_EQUAL_UINT8_ARRAY("WAVE", buf + 8, 4);
    TEST_ASSERT_EQUAL_UINT8_ARRAY("fmt ", buf + 12, 4);
    TEST_ASSERT_EQUAL_UINT8_ARRAY("data", buf + 36, 4);
}

static void test_16khz_mono_16bit_fields_are_correct() {
    uint8_t buf[44];
    const uint32_t dataBytes = 1000;
    wavWriteHeader(buf, 16000, 16, 1, dataBytes);

    TEST_ASSERT_EQUAL_UINT32(36 + dataBytes, readU32(buf + 4));  // RIFF チャンクサイズ
    TEST_ASSERT_EQUAL_UINT16(1, readU16(buf + 20));    // AudioFormat = PCM
    TEST_ASSERT_EQUAL_UINT16(1, readU16(buf + 22));    // channels = mono
    TEST_ASSERT_EQUAL_UINT32(16000, readU32(buf + 24)); // sampleRate
    TEST_ASSERT_EQUAL_UINT32(16000 * 1 * 16 / 8, readU32(buf + 28));  // byteRate
    TEST_ASSERT_EQUAL_UINT16(1 * 16 / 8, readU16(buf + 32));  // blockAlign
    TEST_ASSERT_EQUAL_UINT16(16, readU16(buf + 34));   // bitsPerSample
    TEST_ASSERT_EQUAL_UINT32(dataBytes, readU32(buf + 40));
}

static void test_data_length_reflects_given_bytes() {
    uint8_t buf[44];
    wavWriteHeader(buf, 16000, 16, 1, 32000);
    TEST_ASSERT_EQUAL_UINT32(32000, readU32(buf + 40));
    TEST_ASSERT_EQUAL_UINT32(36 + 32000, readU32(buf + 4));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_header_size_is_44_bytes);
    RUN_TEST(test_riff_wave_magic_and_chunk_ids);
    RUN_TEST(test_16khz_mono_16bit_fields_are_correct);
    RUN_TEST(test_data_length_reflects_given_bytes);
    return UNITY_END();
}
