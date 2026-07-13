# 00014: 対話方式を Realtime API から STT + LLM + TTS パイプラインに変更

## What / Why

gpt-realtime-mini の応答品質(賢さ)に不満があるため、対話を 3 段のパイプラインに変更する:

1. **STT**: マイク録音(発話終了は端末側 VAD で検知)→ `POST /v1/audio/transcriptions`(model: `gpt-4o-mini-transcribe`, language: ja)
2. **LLM**: 会話履歴つきで `POST /v1/chat/completions`(model: `gpt-5-nano`)
3. **TTS**: 応答テキストを `POST /v1/audio/speech`(model: `gpt-4o-mini-tts`, `response_format: pcm` 24kHz)→ 一括再生

単価(1M トークン、2026-07 確認、出典: developers.openai.com/api/docs/pricing):
- gpt-5-nano: in $0.05 / out $0.40
- gpt-4o-mini-transcribe: audio in $3.00 / text out $5.00(text in $1.25)
- gpt-4o-mini-tts: text in $0.60 / audio out $12.00

音声 in/out とも realtime-mini($10/$20)より大幅に安い。トレードオフは応答レイテンシ増(直列 3 リクエスト)。

## 受け入れ基準

- [ ] 端末側 VAD(エネルギーベースの発話開始・終了判定)が純関数 `src/pure/vad.*` に分離されテストされる(検証: native テスト。先に red。ケース: 無音のみ / 発話→無音で終了検知 / 短いノイズでは開始しない / ハングオーバー)
- [ ] WAV ヘッダ生成が純関数 `src/pure/wav.*` でテストされる(検証: native テスト。16bit mono、サンプルレート・データ長がヘッダに正しく入る)
- [ ] chat/completions のリクエスト JSON 組み立て(system + 会話履歴 + 新規発話)とレスポンス解釈(content, usage)が純関数 `src/pure/chat_protocol.*` でテストされる(検証: native テスト。先に red)
- [ ] 金額換算に 3 モデルの単価を追加し、1 ターン(STT usage + chat usage + TTS 推定)の円換算がテストされる(検証: native テスト `test_usage_cost` 拡張)
- [ ] 実機: タップ → 発話 → 数秒で音声応答が再生され、会話テキスト(あなた / kikimimi)と累計金額が画面に表示される(検証: 実機で会話)
- [ ] 実機: 連続対話で文脈が維持される(直前の発話を踏まえた応答。検証: 実機で「今の話を要約して」等)
- [ ] cores3 ビルドが通り、native テストが全 green(検証: `uvx --from platformio pio run -e cores3` / `pio test -e native`)

## やらないこと

- Realtime API クライアント(`src/realtime_client.*`)の削除(切り戻し比較用に残す。main からの参照だけ外す)
- ストリーミング TTS 再生・応答の逐次表示(まず全量受信 → 一括再生。レイテンシ改善は動いてから)
- ウェイクワード・常時待受
- TTS の音声キャッシュ

## How(実装方針)

- `src/voice_pipeline.h/.cpp`(新規): 状態機械は既存と同じ語彙(Idle/Listening/Thinking/Speaking/ErrorState)+ 同形のコールバック(onStateChanged/onError/onCostUpdated/onTranscriptUpdated)にし、main.cpp の変更を差し替え程度に抑える
- 録音: 16kHz mono 16bit で PSRAM に貯めつつ、チャンクごとの RMS を `pure/vad.*` に渡して発話終了(例: 発話後 700ms 無音)を検知。上限 15 秒。開始直後から無音のままなら聞き直しへ
- HTTP: HTTPClient + WiFiClientSecure。multipart/form-data で WAV を送る(STT)。TTS の PCM は PSRAM に受けて `M5.Speaker.playRaw`(24kHz)。30 秒上限は踏襲
- 会話履歴: RAM 上の vector(セッション中のみ、タップ終了でクリア)。system プロンプトは既存 SESSION_INSTRUCTIONS
- 金額: STT / chat は各レスポンスの usage、TTS は usage が返る場合はそれを、返らない場合は入力文字数と受信 PCM 長からの概算(概算であることを result.md に注記)
- `main.cpp`: realtime_client の呼び出しを voice_pipeline に差し替え
- **ADR**: 対話方式の変更(Realtime API → パイプライン)は今後の前提になるため `docs/decisions/` に追記(理由: 応答品質、副次: コスト減・レイテンシ増)
- 前作 `/Users/tokumei/Work/yesman/` は STT/TTS パイプライン型ロボットだったはずなので、実装・knowhow に流用できるものがないか最初に確認する
- TTS voice はまず既定候補(例: marin が使えなければ coral / sage 等)をビルド前に API ドキュメントで確認する
