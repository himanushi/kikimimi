# 00009: 会話内容の画面表示(トランスクリプト)

## What / Why

対話中、いま何を話しているのか画面で確認できるようにする。Realtime API のトランスクリプトイベントを受けて、対話画面にユーザー発話とアシスタント応答のテキストを表示する。

- アシスタント側: `response.output_audio_transcript.delta` / `response.output_audio_transcript.done`(音声応答の書き起こしが無料で流れてくる)
- ユーザー側: `session.update` の `audio.input.transcription` に `{"model":"gpt-4o-mini-transcribe"}` を設定すると `conversation.item.input_audio_transcription.completed` が届く

依存: 00008(ホーム画面刷新)の後に実施。対話画面のレイアウトはこの issue で整える。

## 受け入れ基準

- [ ] `parseServerEvent` が `response.output_audio_transcript.delta`(delta 抽出)と `conversation.item.input_audio_transcription.completed`(transcript 抽出)を解釈する(検証: native テスト。先に red を確認)
- [ ] `buildSessionUpdateEvent` の JSON に `input.transcription.model = "gpt-4o-mini-transcribe"` が含まれる(検証: native テスト。先に red)
- [ ] 会話履歴の保持・行折り返し・表示行の選択(最新 N 行)が純関数 `src/pure/transcript_view.*` に分離されテストされる(検証: `uvx --from platformio pio test -e native` green)
- [ ] 対話中の画面に、ユーザー発話(例: 「あなた: …」)とアシスタント応答(例: 「kikimimi: …」、delta 逐次追記)が表示される(検証: 実機で会話して目視)
- [ ] 状態ラベルと累計金額(¥)の表示は維持される(検証: 実機)
- [ ] cores3 ビルドが通る(検証: `uvx --from platformio pio run -e cores3` SUCCESS)

## やらないこと

- 会話履歴の永続化・タッチスクロール(表示は直近数往復のみ、セッション終了で破棄)
- 入力書き起こし(gpt-4o-mini-transcribe)の費用の金額表示への加算(response.done の usage に載らないため。表示額が僅かに実費より低くなる点は result.md に注記)
- ポータル・ホーム画面の変更

## How(実装方針)

- `src/pure/realtime_protocol.*`: ServerEventType に `OutputAudioTranscriptDelta` / `InputTranscriptionCompleted` を追加、`buildSessionUpdateEvent` に transcription 設定を追加
- `src/pure/transcript_view.h/.cpp`(新規): 発話単位の履歴(話者 + テキスト)を持ち、delta 追記・確定・「画面幅 N 文字で折り返した最新 M 行」の取得を純関数で。UTF-8(日本語 3 バイト文字)の折り返しに注意
- `src/realtime_client.*`: コールバック `onTranscriptUpdated` を追加し、delta / completed で通知
- `src/main.cpp`: 対話画面の描画をトランスクリプト表示つきに変更(上部: 状態 + ¥、下部: 会話テキスト。efontJA_16)
- delta ごとの全画面再描画はチラつくので、テキスト領域のみ再描画するなど頻度・範囲を抑える(実装者の裁量)
- 参考: `knowhow/realtime-api-ga-event-schema.md`
