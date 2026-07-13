// 対話画面の会話テキスト表示(issue 00009)。
// 発話単位の履歴の保持と、画面表示用の折り返し・行選択。デバイス非依存の純関数(native 環境でテストする)
#pragma once

#include <string>
#include <vector>

namespace transcript_view {

enum class Speaker {
    User,
    Assistant,
};

struct Utterance {
    Speaker speaker;
    std::string text;
    bool finalized = false;
};

using History = std::vector<Utterance>;

// アシスタントの音声書き起こし delta を追記する。末尾が未確定のアシスタント発話ならそこに連結し、
// そうでなければ新規発話として追加する
History appendAssistantDelta(History history, const std::string& delta);

// 末尾のアシスタント発話を確定する(response.done 到達時)。末尾がアシスタント発話でなければ何もしない
History finalizeAssistant(History history);

// ユーザー発話の確定テキストを新規発話として追加する
History appendUserUtterance(History history, const std::string& text);

// 発話ごとに話者ラベルを付け、wrapWidth(コードポイント数)で折り返した表示行を、
// 末尾から maxLines 行だけ返す。UTF-8 マルチバイト文字の途中では折り返さない
std::vector<std::string> renderLines(const History& history, int wrapWidth, int maxLines);

}  // namespace transcript_view
