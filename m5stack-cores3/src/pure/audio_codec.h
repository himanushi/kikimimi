// PCM16 音声チャンクと base64 の相互変換(デバイス非依存の純関数)。
// input_audio_buffer.append の送信・response.output_audio.delta の受信で使う
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace audio_codec {

// バイト列を base64 エンコードする
std::string encodeBase64(const uint8_t* data, size_t len);

// base64 文字列をデコードする。不正な文字を含む場合は空を返す(壊れたフレーム対策)
std::vector<uint8_t> decodeBase64(const std::string& base64);

}  // namespace audio_codec
