# OpenAI Realtime API は GA で beta 版とイベント名・スキーマが変わっている

- **症状**: beta 版(`OpenAI-Beta: realtime=v1` ヘッダを使う旧仕様)の記事・サンプルコードは `session.update` の `modalities` フィールドや `response.text.delta` イベント名を使っているが、GA(`gpt-realtime` モデル)ではそのまま送っても `session.updated` が返らない・想定イベントが来ない可能性がある
- **GA での差分**:
  - `session.update` の `session` オブジェクトに `type: "realtime"` が必要
  - 応答モダリティ指定は `modalities` でなく `output_modalities`(値は `["text"]` や `["audio"]`)
  - テキストのストリーミングイベント名は `response.output_text.delta`(beta の `response.text.delta` ではない)
  - 音声出力設定は `session.audio.output` 配下に移動(beta は session 直下)
  - voice・model は音声出力後は変更不可
- **対処**: 実装前に `platform.openai.com/docs/api-reference/realtime-client-events` / `realtime-server-events` の GA 版を確認する。古い検索結果・ブログは beta 版の可能性が高い
- **出典**: issue 00002(`src/pure/realtime_protocol.cpp`)
