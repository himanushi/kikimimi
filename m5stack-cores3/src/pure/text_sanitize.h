// LLM 応答テキストの読み上げ用サニタイズ(issue 00015 の後追い修正)。
// web_search 有効時、instructions で禁止しても URL・出典が本文に混ざることがあるため、
// TTS・画面表示の前にプログラム側で確実に除去する。デバイス非依存(native でテストする)
#pragma once

#include <string>

namespace text_sanitize {

// markdown リンクはラベルだけ残し、生 URL は除去する。URL だけを括った括弧
// (半角・全角)は括弧ごと消す。余った連続スペース・句読点前のスペースは詰める
std::string sanitizeForSpeech(const std::string& text);

}  // namespace text_sanitize
