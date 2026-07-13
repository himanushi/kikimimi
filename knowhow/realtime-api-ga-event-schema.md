# OpenAI Realtime API は GA で beta 版とイベント名・スキーマが変わっている

- **症状**: beta 版(`OpenAI-Beta: realtime=v1` ヘッダを使う旧仕様)の記事・サンプルコードは `session.update` の `modalities` フィールドや `response.text.delta` イベント名を使っているが、GA(`gpt-realtime` モデル)ではそのまま送っても `session.updated` が返らない・想定イベントが来ない可能性がある
- **GA での差分**:
  - `session.update` の `session` オブジェクトに `type: "realtime"` が必要
  - 応答モダリティ指定は `modalities` でなく `output_modalities`(値は `["text"]` や `["audio"]`)
  - テキストのストリーミングイベント名は `response.output_text.delta`(beta の `response.text.delta` ではない)
  - 音声出力設定は `session.audio.output` 配下に移動(beta は session 直下)
  - voice・model は音声出力後は変更不可
- **対処**: 実装前に `platform.openai.com/docs/api-reference/realtime-client-events` / `realtime-server-events` の GA 版を確認する。古い検索結果・ブログは beta 版の可能性が高い(`platform.openai.com` 配下は WebFetch が 403 で読めないことがあり、その場合は `developers.openai.com/api/docs/guides/realtime-conversations` `realtime-vad` などのガイドページ・WebSearch のスニペットで代替する)
- **音声関連の GA スキーマ(issue 00003 で確認)**:
  - 音声フォーマットは `session.audio.input.format` / `session.audio.output.format` に `{"type": "audio/pcm", "rate": 24000}`(24kHz 固定、文字列 `"pcm16"` ではない)
  - VAD は `session.audio.input.turn_detection.type` に `"server_vad"` または `"semantic_vad"`
  - マイク送信は `input_audio_buffer.append` の `audio` フィールドに base64 化した PCM16 チャンク(15MB/イベント上限)
  - 応答音声受信は `response.output_audio.delta` の `delta` フィールド(base64)。テキストと同じく `delta` という名前
  - 発話区間検出は `input_audio_buffer.speech_started` / `speech_stopped`(追加フィールドなし)
  - `output_modalities` に `"audio"` と `"text"` を両方指定して両方受け取る挙動は GA で不安定という報告がある(コミュニティ報告)。テキストと音声を同時に使う設計なら要検証
- **出典**: issue 00002 / issue 00003(`src/pure/realtime_protocol.cpp`)
