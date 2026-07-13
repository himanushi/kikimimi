# 00015: LLM 段に web_search ツールを追加(Responses API へ移行)

## What / Why

会話中の事実に関する質問で適当な答えを返さないよう、モデルが必要と判断したときに API 側でウェブ検索して答えられるようにする。OpenAI Responses API(`POST /v1/responses`)の組み込み `web_search` ツールを使う。モデルは現行の gpt-5-nano のまま。

- 呼び先を `/v1/chat/completions` → `/v1/responses` に変更し、`tools: [{"type":"web_search"}]` を宣言
- 検索するかどうかはモデル任せ(雑談では検索されず追加コストなし)
- 音声で読み上げる都合上、URL・引用元の羅列を応答文に含めないよう instructions を調整する

## 受け入れ基準

- [ ] Responses API のリクエスト JSON 組み立て(model / instructions / 会話履歴 input / web_search ツール宣言)とレスポンス解釈(output 配列から output_text の連結、usage の input/output/cached トークン)が純関数でテストされる(検証: native テスト。先に red)
- [ ] レスポンスに web_search_call アイテムが混ざっていても正しく本文だけ抽出できる(検証: native テストにモック JSON)
- [ ] 金額計算: usage(input/cached/output)ベースの円換算が引き続き機能する。検索ツール自体の従量課金は概算対象外とし、その旨を result.md に注記(検証: native テスト)
- [ ] 実機: 「今日の東京の天気は?」等の検索が必要な質問に、最新情報を踏まえた応答が返る(検証: 実機で会話 + ログ)
- [ ] 実機: 雑談(検索不要な発話)は従来どおりのレイテンシで応答する(検証: 実機 + [pipeline] ログの chat 所要時間)
- [ ] cores3 ビルド・native テスト全 green(検証: `uvx --from platformio pio run -e cores3` / `pio test -e native`)

## やらないこと

- 引用(citation)の画面表示(まず音声対話として成立させる)
- Anthropic API 対応・モデルの mini 格上げ(別判断)
- 検索結果のキャッシュ

## How(実装方針)

- `pure/chat_protocol.*` を Responses API 形式に変更(または `responses_protocol` として追加し chat_protocol を置き換え)。リクエスト: `{model, instructions, input: [{role, content}...], tools: [{"type": "web_search"}]}`。レスポンス: `output[]` から `type=="message"` の `content[].type=="output_text"` の text を連結。usage は `input_tokens` / `output_tokens` / `input_tokens_details.cached_tokens`
- `voice_pipeline.cpp`: エンドポイントと組み立て・解釈の呼び替え。検索ターンは時間がかかるため CHAT_TIMEOUT_MS を 30 秒に拡大
- `main.cpp` の SESSION_INSTRUCTIONS に「音声で読み上げるため URL や出典の列挙はしない。最新情報が必要なら検索してよい」を追記
- 実装前に developers.openai.com で Responses API の web_search ツール仕様(gpt-5-nano での可否・リクエスト形)を WebFetch で確認する。gpt-5-nano で web_search が使えない場合は手を止めて報告(勝手にモデルを変えない)
