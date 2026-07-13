# 00003 実装報告

## 検証の証跡

- [x] 音声チャンクの組み立て・分解が純関数で分離され、ホスト側テストが通る
  - `m5stack-cores3/src/pure/audio_codec.h/.cpp`(base64 エンコード/デコード)、`m5stack-cores3/src/pure/realtime_protocol.h/.cpp`(`buildInputAudioAppendEvent` / `ResponseOutputAudioDelta` / `SpeechStarted` / `SpeechStopped` のパース、音声対応した `buildSessionUpdateEvent`)に分離
  - `uvx --from platformio pio test -e native` で 42 件全 PASSED(`test_audio_codec` 7 件、`test_realtime_protocol` 15 件を含む)を確認
- [x] ビルドが通る
  - `uvx --from platformio pio run -e cores3` で `SUCCESS`(RAM 15.8%, Flash 28.7%)を確認
- [ ] タップで会話開始 → 話しかけると音声で応答が返る — **未検証(実機書き込み待ち)**
- [ ] 応答再生中/再生後に続けて話せる(3 往復以上の連続対話) — **未検証(実機書き込み待ち)**
- [ ] 再タップで会話終了し、待機状態に戻る — **未検証(実機書き込み待ち)**
- [ ] 会話中の状態(聞いている/考えている/話している)が画面で分かる — 画面表示ロジック(`realtimeStateLabel` の Listening/Thinking/Speaking 分岐)は実装済み。表示が実機で正しく切り替わるかは**未検証(実機書き込み待ち)**

## 判断と経緯

- **音声フォーマット・VAD・再生アーキテクチャ**は今後の音声関連 issue の前提になると判断し `docs/decisions/00003-realtime-audio-format-and-turn-detection.md` として ADR 化した(PCM16 24kHz 固定、semantic_vad、ターンごとに全量受信してから 1 回で再生)
- plan.md は「受信 delta は PSRAM のリングバッファに貯めて再生」としていたが、`M5.Speaker.playRaw()` を同一チャンネルへ小刻みに連続呼び出ししたときのキューイング挙動(音切れ・上書きの有無)を実機なしに確認する手段がなく、確実に動作する「ターンごとに全量貯めてから 1 回で再生」に変更した。理由は ADR と `knowhow/m5stack-speaker-playraw-queueing-unverified.md` に記録
- `RealtimeState` を issue 00002 の `Established` 一本から `Listening` / `Thinking` / `Speaking` の 3 状態に分割した。plan.md の受け入れ基準「聞いている/考えている/話している が画面で分かる」を満たすための直接の要求であり、スコープ逸脱ではない
- 00002 で実装したテキスト往復用の `realtimeSendUserMessage` / `onResponseTextDelta` コールバック・固定挨拶メッセージ送信は削除した。00003 で `output_modalities` を `["audio"]` のみに変更したため(コミュニティ報告のある text+audio 同時指定の不安定さを避けた。`knowhow/realtime-api-ga-event-schema.md` 参照)、テキスト応答経路は使われなくなる。会話は VAD 駆動(タップ後に無言で待ち、ユーザーが話し始めたら `speech_started` で拾われる)に切り替わり、固定挨拶の送信は不要になった
- 応答音声バッファは 30 秒分(PSRAM, 約 1.4MB)を固定確保し、超過分は切り捨てる設計にした。長い応答は現状のスコープでは想定していないため、まず動く上限を決めて先に進んだ

## レビュー指摘と対応

フレッシュな subagent(sonnet)に `issues/00003/plan.md` と diff のみを渡してレビューを実施。high/medium の指摘はなし。

- low: `beginSpeaking()` の `playbackLen == 0` 時のフォールバック(黙って Listening に戻す)が「起こり得ないシナリオへの防御コード」寄りではないかという指摘 → 応答が音声を伴わないケース(エラー応答等)で無限に Thinking のまま固まるのを防ぐ実用的なガードと判断し、対応不要とした
- low: diff だけではビルド可否が確認できないという指摘 → `pio run -e cores3` の実行結果(SUCCESS)は本記録の「検証の証跡」に記載済み

## 未検証項目(実機書き込み待ち)

- タップでの会話開始・音声応答・レイテンシ
- 連続対話(3 往復以上)
- 再タップでの終了・待機状態への復帰
- 状態表示(聞いている/考えている/話している)の実機での見え方
- Mic/Speaker 排他制御が実機で意図通りに動くか(特に `Speaking` → `Listening`復帰時の `M5.Speaker.isPlaying()` 判定)
- `knowhow/m5stack-speaker-playraw-queueing-unverified.md` に記録した `playRaw()` のキューイング挙動
