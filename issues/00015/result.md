# 00015: 結果

## 検証の証跡

- `pure/chat_protocol.*`(Responses API の組み立て・解釈): `test/test_chat_protocol/test_chat_protocol.cpp` を Responses API 仕様に書き換えて先に red を確認(`instructions` フィールド分離・`web_search` ツール宣言・`output[]` の `message` アイテムからの `output_text` 抽出が未実装で失敗)、その後 `src/pure/chat_protocol.cpp` を実装して green。ケース: `instructions` が `input` 配列に system ロールとして混ざらないこと、`tools:[{"type":"web_search"}]` の宣言、会話履歴が新規発話より前に積まれること、引用符エスケープ、`output[]` に `web_search_call` アイテムが混在していても `message` アイテムの `output_text` だけを連結して抽出できること、複数 `output_text` パートの連結、`usage`(`input_tokens` / `output_tokens` / `input_tokens_details.cached_tokens`)の抽出、壊れた JSON・`message` アイテム欠如時に `ok=false` になること
- 自己検証: `uvx --from platformio pio test -e native` で 147 件全 green(既存 136 件 + 追加・変更 11 件)。`uvx --from platformio pio run -e cores3` は SUCCESS(Flash 28.7%, RAM 16.0%)
- 金額計算: `usage_cost::calculateChatCostJpy` はトークン数のみを引数に取る純関数で、エンドポイントの違い(chat/completions → responses)に依存しないため無改修で継続動作する。**web_search ツール自体の従量課金(1 呼び出しあたりの課金)は概算対象外**とした。現状の実装は応答本文の `usage`(input/output/cached tokens)のみを円換算しており、検索ツールの呼び出し課金は反映されない
- 実機確認が必要な受け入れ基準(未検証、実機確認待ち):
  - 検索が必要な質問(「今日の東京の天気は?」等)に最新情報を踏まえた応答が返るか
  - 雑談(検索不要な発話)が従来どおりのレイテンシで応答するか([pipeline] ログの chat 所要時間で確認)

## 判断と経緯

- **gpt-5-nano の web_search 対応可否**: plan.md の指示に従い developers.openai.com を確認したが、モデルページ・web_search ガイドのいずれにも gpt-5-nano への対応・非対応の明記がなかった。ガイド内の表に載っているのは `gpt-5-search-api` / `gpt-4o-search-preview` / `gpt-4o-mini-search-preview` / `gpt-4.1` / `gpt-4.1-mini` / `o4-mini` のみだが、文脈上これは網羅的な対応表ではなく「制限事項(コンテキスト長)」の表に見えた。一方、OpenAI API を仲介する Cloudflare AI Gateway の公開ドキュメントは gpt-5-nano を web_search 対応モデルとして明示的に列挙しており、非対応と明記されているのは gpt-4.1-nano / o1-pro / o3-mini だった。gpt-5-nano + web_search で invalid_request_error になるという実例も見つからなかった。「使えない」と確定できる一次情報がなかったため実装を進めた。最終的な可否は実機での受け入れ基準確認で実証する。動かなければモデル変更はせず、その旨をここに追記して報告する
- **`chat_protocol.*` は新規追加でなく置き換え**: plan.md の How には「`responses_protocol` として追加」の選択肢も示されていたが、`Message` / `History` / `Usage` / `ChatResponse` の型・関数シグネチャをそのまま使い回せたため、ファイル名・呼び出し側(`voice_pipeline.cpp`)を変えずに内部実装だけを Responses API 形式へ置き換えた。呼び出し側の差分を最小化できるため
- **system instructions は `input` 配列に混ぜず `instructions` フィールドへ**: `../yesman/knowhow/prompt-ai-sdk-v7-system-via-instructions.md` に記録済みの知見(OpenAI 系 API は system ロールを messages/input に混ぜることを許可しないケースがある)と、Responses API の仕様(instructions は独立フィールド)に合わせた
- **CHAT_TIMEOUT_MS を 20 秒→30 秒に拡大**: plan.md の指示通り。web_search 実行時は検索分の待ち時間が chat 呼び出しに乗るため

## レビュー指摘と対応

フレッシュな subagent に diff と plan.md のみを渡してレビューを依頼した。重大な指摘はなかった。レビュー時点で `result.md` が未作成だった点と、レビュー担当自身は cores3 ビルド・実機検証を行っていない旨の指摘があったが、cores3 ビルドは実装側で `uvx --from platformio pio run -e cores3` を実行し SUCCESS を確認済みで、実機検証は元々「未検証(実機確認待ち)」として本ドキュメントに記録する方針だった。追加の是正は不要と判断した。
