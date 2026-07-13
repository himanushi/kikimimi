// WAV(RIFF)ヘッダ生成。デバイス非依存の純関数(native 環境でテストする)。
// STT へ送る録音済み PCM(16kHz mono 16bit)に付与する
#pragma once
#include <stddef.h>
#include <stdint.h>

constexpr size_t WAV_HEADER_SIZE = 44;

// out に 44 byte の RIFF/PCM ヘッダを書き、書いたバイト数(= 44)を返す。
// dataBytes は後続 PCM データのバイト長
size_t wavWriteHeader(uint8_t* out, uint32_t sampleRate, uint16_t bitsPerSample,
                      uint16_t channels, uint32_t dataBytes);
