# 00004: 対話方式を Realtime API から STT + LLM + TTS パイプラインへ変更

## 文脈

issue 00002〜00013 で OpenAI Realtime API(WebSocket, `gpt-realtime-mini`)による音声対話を実装してきたが、応答の賢さ(会話の質)に不満が出た。`gpt-realtime-mini` は会話特化のモデルで、テキストのみの `chat/completions` で使える上位モデル群(`gpt-5-nano` 等)と比べて推論力が劣る。この選択は今後の対話関連 issue(会話履歴・知識蓄積・音声品質)すべての前提になるため ADR とする。

## 決定

対話を 3 段の直列 HTTP パイプラインに変更する:

1. **STT**: マイク録音(16kHz mono 16bit、端末側のエネルギーベース VAD で発話終了を検知)→ `POST /v1/audio/transcriptions`(`gpt-4o-mini-transcribe`, `language: ja`)
2. **LLM**: system instructions + 会話履歴 + 新規発話で `POST /v1/chat/completions`(`gpt-5-nano`)
3. **TTS**: 応答テキストを `POST /v1/audio/speech`(`gpt-4o-mini-tts`, `response_format: pcm` 24kHz)→ 全量受信後に `M5.Speaker.playRaw` で一括再生

`realtime_client.*`(WebSocket 実装)は削除せず残す。切り戻し・比較のためで、main.cpp からの呼び出しだけを `voice_pipeline.*` に差し替えた。

## 選ばなかった選択肢

- **Realtime API のモデルだけ上位に変更**: 2026-07 時点で `gpt-realtime`(mini でない上位版)は存在するが、音声対話特化モデルである点は変わらず、テキスト推論力そのものは `gpt-5-nano` 等の主力チャットモデル系列に劣る。応答品質の不満はモデル系統の差によるものと判断した
- **Realtime API を維持しつつサーバ側で LLM を差し替える中継構成**: `server/`(知識蓄積・中継)がまだ存在しないため、デバイス単体で完結する構成を優先した。中継サーバを作る段になったら、STT/TTS もサーバ側に寄せるか再検討する

## 帰結

- **コスト減**: 音声 in/out ともパイプライン側が Realtime API(mini, $10/$20 per 1M tokens 相当)より大幅に安い(pricing は `issues/00014/plan.md` 参照)
- **レイテンシ増**: 直列 3 HTTP リクエストのため、Realtime API の低遅延ストリーミング応答より体感の間が空く。実機でのレイテンシは未検証(`issues/00014/result.md` に記録)
- **VAD が端末側のエネルギーベースに変わる**: Realtime API の `semantic_vad`(言い淀みに強い)から、閾値・ハングオーバーによる単純なエネルギー VAD に変わる。誤検知(短いノイズでの誤起動、無音のみでの聞き逃し)は実機での閾値調整が必要になる可能性がある
- ストリーミング再生・ウェイクワード・TTS 音声キャッシュは引き続き対象外(00003 の帰結を維持)
- 対話の「賢さ」が実際に改善したかは実機での主観評価が必要(検証方法が定量化しにくいため、今回は plan.md の受け入れ基準どおり実機確認で判断する)
