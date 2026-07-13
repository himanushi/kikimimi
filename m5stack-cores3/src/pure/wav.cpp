#include "wav.h"

#include <string.h>

static void putU16(uint8_t* p, uint16_t v) {
    p[0] = v & 0xFF;
    p[1] = (v >> 8) & 0xFF;
}

static void putU32(uint8_t* p, uint32_t v) {
    p[0] = v & 0xFF;
    p[1] = (v >> 8) & 0xFF;
    p[2] = (v >> 16) & 0xFF;
    p[3] = (v >> 24) & 0xFF;
}

size_t wavWriteHeader(uint8_t* out, uint32_t sampleRate, uint16_t bitsPerSample,
                      uint16_t channels, uint32_t dataBytes) {
    const uint16_t blockAlign = channels * bitsPerSample / 8;
    const uint32_t byteRate = sampleRate * blockAlign;

    memcpy(out, "RIFF", 4);
    putU32(out + 4, 36 + dataBytes);
    memcpy(out + 8, "WAVEfmt ", 8);
    putU32(out + 16, 16);  // fmt チャンク長(PCM 固定)
    putU16(out + 20, 1);   // AudioFormat = PCM
    putU16(out + 22, channels);
    putU32(out + 24, sampleRate);
    putU32(out + 28, byteRate);
    putU16(out + 32, blockAlign);
    putU16(out + 34, bitsPerSample);
    memcpy(out + 36, "data", 4);
    putU32(out + 40, dataBytes);
    return WAV_HEADER_SIZE;
}
