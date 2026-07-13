#include "transcript_view.h"

#include <algorithm>

namespace transcript_view {

namespace {

// UTF-8 バイト列をコードポイント単位の断片に分割する(先頭バイトのビットパターンで長さを判定)
std::vector<std::string> splitCodepoints(const std::string& s) {
    std::vector<std::string> out;
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        size_t len = 1;
        if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;
        len = std::min(len, s.size() - i);
        out.push_back(s.substr(i, len));
        i += len;
    }
    return out;
}

std::string speakerLabel(Speaker speaker) {
    return speaker == Speaker::User ? "あなた: " : "kikimimi: ";
}

}  // namespace

History appendAssistantDelta(History history, const std::string& delta) {
    if (!history.empty() && history.back().speaker == Speaker::Assistant && !history.back().finalized) {
        history.back().text += delta;
        return history;
    }
    Utterance u;
    u.speaker = Speaker::Assistant;
    u.text = delta;
    u.finalized = false;
    history.push_back(u);
    return history;
}

History finalizeAssistant(History history) {
    if (!history.empty() && history.back().speaker == Speaker::Assistant) {
        history.back().finalized = true;
    }
    return history;
}

History appendUserUtterance(History history, const std::string& text) {
    Utterance u;
    u.speaker = Speaker::User;
    u.text = text;
    u.finalized = true;
    history.push_back(u);
    return history;
}

std::vector<std::string> renderLines(const History& history, int wrapWidth, int maxLines) {
    std::vector<std::string> allLines;
    for (const auto& u : history) {
        std::string full = speakerLabel(u.speaker) + u.text;
        auto codepoints = splitCodepoints(full);
        std::string line;
        int count = 0;
        for (const auto& cp : codepoints) {
            if (count >= wrapWidth) {
                allLines.push_back(line);
                line.clear();
                count = 0;
            }
            line += cp;
            count++;
        }
        allLines.push_back(line);
    }
    if (static_cast<int>(allLines.size()) <= maxLines) return allLines;
    return std::vector<std::string>(allLines.end() - maxLines, allLines.end());
}

}  // namespace transcript_view
