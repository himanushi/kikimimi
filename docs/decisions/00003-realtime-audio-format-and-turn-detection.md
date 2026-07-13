# 00003: リアルタイム音声対話の音声フォーマット・VAD 方式・再生アーキテクチャ

## 文脈

issue 00003 でマイク音声のストリーミング送信とスピーカー再生を実装するにあたり、次の 3 点は今後の音声関連 issue(サーバ中継・知識蓄積・音声品質チューニング)でも前提になる選択だった。

1. 音声フォーマット・サンプルレート
2. 発話の切れ目検出(VAD)方式
3. 応答音声の受信〜再生のアーキテクチャ(M5Unified は Mic/Speaker 排他)

## 決定

### 1. 音声フォーマット: PCM16, 24kHz 固定

Realtime API GA では `session.audio.input.format` / `session.audio.output.format` に `{"type": "audio/pcm", "rate": 24000}` を指定する(24kHz のみサポート、公式ドキュメント `developers.openai.com/api/docs/guides/realtime-vad` で確認)。M5Unified の Mic/Speaker のサンプルレートもこれに合わせて 24000 で統一する。g711 8kHz への切り替えは検討したが、帯域が問題になってから対応すればよく、初期実装では選ばない。

### 2. VAD 方式: semantic_vad

`session.audio.input.turn_detection.type` に `semantic_vad` を指定。`server_vad`(音量ベース、閾値・無音時間で判定)より、発話内容から「言い終えたか」を推定する `semantic_vad` の方が「あー」「えーと」のような言い淀みで誤って区切られにくく、対話アプリの体験に合う。`eagerness` は指定せず `auto`(= `medium` 相当)のデフォルトに任せる。

### 3. 再生アーキテクチャ: ターンごとに全量受信してから 1 回で再生

`response.output_audio.delta` を PSRAM 上の固定長バッファ(30 秒分)に貯め、`response.done` を受けてから `M5.Speaker.playRaw()` を 1 回呼んで再生する。plan.md は「PSRAM のリングバッファに貯めて再生」としていたが、M5Unified の `Speaker.playRaw()` を同一チャンネルに小刻みに連続呼び出ししたときの内部キューイング挙動(音切れ・上書きの有無)が実機検証なしには確認できないため、確実に動作する「貯めてから再生」を採用した。応答が始まってから音が出るまでの遅延は増えるが、まず動く状態を優先する(plan.md の「音声品質のチューニングは動いてから」と整合)。真のストリーミング再生(delta ごとの逐次再生)は、実機でのキューイング挙動確認とセットで別 issue に切り出す。

## 帰結

- 応答音声は 30 秒を超えると切り捨てられる(超過分は破棄、クラッシュはしない)。長い応答が実用上問題になったらバッファ拡大かストリーミング再生への切り替えを検討する
- Mic と Speaker の排他は「聞く(Listening)/ 考える(Thinking)/ 話す(Speaking)」の 3 状態のステートマシンで明示的に切り替える(`m5stack-cores3/src/realtime_client.cpp`)。barge-in(発話中の割り込み)はこの設計では作れないが、issue 00003 の「やらないこと」で明示的に対象外
- 実機での対話品質(レイテンシ・音切れ)は未検証。実機書き込み後に確認し、問題があればこの ADR を見直す
