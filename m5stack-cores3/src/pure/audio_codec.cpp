#include "audio_codec.h"

namespace audio_codec {

namespace {

constexpr char TABLE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int decodeChar(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

}  // namespace

std::string encodeBase64(const uint8_t* data, size_t len) {
    std::string out;
    if (len == 0) return out;
    out.reserve(((len + 2) / 3) * 4);
    size_t i = 0;
    for (; i + 3 <= len; i += 3) {
        uint32_t n = (static_cast<uint32_t>(data[i]) << 16) |
                     (static_cast<uint32_t>(data[i + 1]) << 8) | data[i + 2];
        out += TABLE[(n >> 18) & 0x3F];
        out += TABLE[(n >> 12) & 0x3F];
        out += TABLE[(n >> 6) & 0x3F];
        out += TABLE[n & 0x3F];
    }
    size_t remaining = len - i;
    if (remaining == 1) {
        uint32_t n = static_cast<uint32_t>(data[i]) << 16;
        out += TABLE[(n >> 18) & 0x3F];
        out += TABLE[(n >> 12) & 0x3F];
        out += "==";
    } else if (remaining == 2) {
        uint32_t n = (static_cast<uint32_t>(data[i]) << 16) | (static_cast<uint32_t>(data[i + 1]) << 8);
        out += TABLE[(n >> 18) & 0x3F];
        out += TABLE[(n >> 12) & 0x3F];
        out += TABLE[(n >> 6) & 0x3F];
        out += '=';
    }
    return out;
}

std::vector<uint8_t> decodeBase64(const std::string& base64) {
    std::vector<uint8_t> out;
    if (base64.empty() || base64.size() % 4 != 0) return out;

    size_t padding = 0;
    if (base64.size() >= 2) {
        if (base64[base64.size() - 1] == '=') padding++;
        if (base64[base64.size() - 2] == '=') padding++;
    }
    out.reserve((base64.size() / 4) * 3);

    for (size_t i = 0; i < base64.size(); i += 4) {
        int vals[4];
        for (int j = 0; j < 4; j++) {
            char c = base64[i + j];
            if (c == '=') {
                vals[j] = 0;
                continue;
            }
            int v = decodeChar(c);
            if (v < 0) return {};  // 不正な文字は壊れたフレームとして扱う
            vals[j] = v;
        }
        uint32_t n = (static_cast<uint32_t>(vals[0]) << 18) | (static_cast<uint32_t>(vals[1]) << 12) |
                     (static_cast<uint32_t>(vals[2]) << 6) | static_cast<uint32_t>(vals[3]);
        out.push_back(static_cast<uint8_t>((n >> 16) & 0xFF));
        if (!(i + 4 == base64.size() && padding >= 2)) out.push_back(static_cast<uint8_t>((n >> 8) & 0xFF));
        if (!(i + 4 == base64.size() && padding >= 1)) out.push_back(static_cast<uint8_t>(n & 0xFF));
    }
    return out;
}

}  // namespace audio_codec
