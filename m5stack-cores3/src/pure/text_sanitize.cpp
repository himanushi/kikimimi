#include "text_sanitize.h"

namespace text_sanitize {

namespace {

constexpr const char* FULLWIDTH_OPEN_PAREN = "\xEF\xBC\x88";   // (
constexpr const char* FULLWIDTH_CLOSE_PAREN = "\xEF\xBC\x89";  // )

bool startsWithUrl(const std::string& s, size_t i) {
    return s.compare(i, 7, "http://") == 0 || s.compare(i, 8, "https://") == 0;
}

// URL の終端(URL に含めない最初の位置)。マルチバイト文字・空白・閉じ記号で止める
size_t urlEnd(const std::string& s, size_t i) {
    while (i < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 0x20 || c >= 0x80) break;
        if (c == ')' || c == ']' || c == '"' || c == '\'') break;
        ++i;
    }
    return i;
}

bool endsWith(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// URL を消した結果、URL だけを括っていた括弧が空で残る場合は括弧ごと消す
size_t dropSurroundingParens(std::string& out, const std::string& s, size_t afterUrl) {
    if (!out.empty() && out.back() == '(' && afterUrl < s.size() && s[afterUrl] == ')') {
        out.pop_back();
        return afterUrl + 1;
    }
    if (endsWith(out, FULLWIDTH_OPEN_PAREN) && s.compare(afterUrl, 3, FULLWIDTH_CLOSE_PAREN) == 0) {
        out.erase(out.size() - 3);
        return afterUrl + 3;
    }
    return afterUrl;
}

// [ラベル](http...) をラベルだけにできるなら true を返し、i を進めて out に追記する
bool consumeMarkdownLink(const std::string& s, size_t& i, std::string& out) {
    size_t labelEnd = s.find(']', i + 1);
    if (labelEnd == std::string::npos || labelEnd + 1 >= s.size() || s[labelEnd + 1] != '(') return false;
    size_t urlStart = labelEnd + 2;
    if (!startsWithUrl(s, urlStart)) return false;
    size_t close = s.find(')', urlStart);
    if (close == std::string::npos) return false;
    out.append(s, i + 1, labelEnd - (i + 1));
    i = close + 1;
    return true;
}

// 連続スペースを 1 つに、句読点(半角 . ,)前のスペースを削る
std::string tidySpaces(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == ' ' && !out.empty() && out.back() == ' ') continue;
        if ((c == '.' || c == ',') && !out.empty() && out.back() == ' ') out.pop_back();
        out += c;
    }
    return out;
}

}  // namespace

std::string sanitizeForSpeech(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    size_t i = 0;
    while (i < text.size()) {
        if (text[i] == '[' && consumeMarkdownLink(text, i, out)) continue;
        if (startsWithUrl(text, i)) {
            size_t end = urlEnd(text, i);
            i = dropSurroundingParens(out, text, end);
            continue;
        }
        out += text[i];
        ++i;
    }
    return tidySpaces(out);
}

}  // namespace text_sanitize
