# 00005: LLM 段のエンドポイントを Responses API へ移行し web_search を有効化

## 文脈

issue 00014 で LLM 段は `POST /v1/chat/completions`(`gpt-5-nano`)を使っていたが、会話中の事実に関する質問(最新情報が必要なもの)に適当な答えを返すことがあった。OpenAI Responses API(`POST /v1/responses`)は組み込みの `web_search` ツールを宣言でき、モデルが必要と判断した場合のみ検索を行う(雑談では追加コストなし)。この選択は今後 LLM を呼ぶすべての issue(知識蓄積・会話拡張など)の前提になるため ADR とする。

## 決定

- LLM 段の呼び先を `/v1/chat/completions` → `/v1/responses` に変更する。モデルは `gpt-5-nano` のまま
- リクエストは `{model, instructions, input: [{role, content}...], tools: [{"type": "web_search"}]}`。system instructions は `input` 配列に混ぜず `instructions` フィールドで渡す(`chat/completions` の system message 方式から変更)
- レスポンスは `output[]` から `type=="message"` の `content[].type=="output_text"` を連結して本文とする。`web_search_call` 等の他アイテムは無視する
- `usage` は `input_tokens` / `output_tokens` / `input_tokens_details.cached_tokens` から取得し、既存の円換算(`usage_cost::calculateChatCostJpy`)にそのまま渡す
- 音声で読み上げる都合上、SESSION_INSTRUCTIONS に「URL・出典の列挙をしない」旨を追記する

## 選ばなかった選択肢

- **gpt-5-nano で web_search が使えない場合の代替モデルへの格上げ**: plan.md の時点で明示的に対象外とした(別判断)。今回は一次情報で「使えない」と確定できなかったため、モデル変更なしで実装し、実機検証で可否を確認する運用にした
- **引用(citation)の画面表示**: 音声対話としての成立を優先し、まずは本文のみを読み上げる。表示が必要になれば別 issue で検討する
- **検索結果のキャッシュ**: 今回は対象外。頻出質問のキャッシュが必要になれば別途検討する

## 帰結

- **コスト**: 応答本文の usage(input/output/cached tokens)は引き続き円換算されるが、web_search ツール自体の従量課金(1 呼び出しあたりの課金)は概算に含まれない。実際の請求額とはこの分だけ乖離しうる(`issues/00015/result.md` に記録)
- **レイテンシ**: 検索が発生したターンは待ち時間が伸びるため `CHAT_TIMEOUT_MS` を 20 秒→30 秒に拡大した。雑談時のレイテンシは従来どおりのはずだが実機確認が必要
- **gpt-5-nano の web_search 対応可否は一次情報で確定できていない**: OpenAI 公式ドキュメントに明記がなく、実装は「使えない」という確証が得られなかったことを根拠に進めた。実機での検索質問への応答確認(`issues/00015/result.md` の受け入れ基準)が実質的な最終確認になる
