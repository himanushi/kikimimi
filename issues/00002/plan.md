# 00002: Realtime API への WebSocket 接続とセッション確立

## What / Why

NVS に保存した API キー(00001)を使い、OpenAI Realtime API に WebSocket(`wss://api.openai.com/v1/realtime`)で接続してセッションを確立する。画面タップで接続開始 → `session.update` 送信 → サーバイベント受信までを確認できる状態にする。

音声はまだ流さない(00003)。先に「接続・認証・イベントの往復」だけを切り出すことで、TLS メモリ・JSON パースなど ESP32 固有の問題を音声処理と切り離してデバッグできるようにする。

## 受け入れ基準

- [ ] Realtime API のイベント生成・パース(session.update の組み立て、サーバイベントの type 判別)が純関数で分離され、ホスト側テストが通る(検証: `pio test -e native`)
- [ ] ビルドが通る(検証: `pio run -e cores3` の出力)
- [ ] 画面タップで WebSocket 接続が始まり、`session.created` 受信までがシリアルログで確認できる(検証: 実機 + シリアルログ)
- [ ] `session.update`(音声設定・instructions)送信後に `session.updated` を受信する(検証: シリアルログ)
- [ ] テキストの会話アイテム送信 → `response.output_text.delta` 系イベントで応答テキストを受信し、画面に表示する(検証: 実機観察 + シリアルログ)
- [ ] 再タップまたは切断でセッションを終了し、再タップで再接続できる(検証: 実機観察)
- [ ] API キーが未登録・無効の場合は画面にエラー表示する(検証: 実機でわざと無効キーを登録)

## やらないこと

- 音声の送受信(00003)
- 会話履歴の保存・知識蓄積
- 自動再接続・指数バックオフ(切れたらタップで再接続、でよい)

## How(実装方針)

- 依存: issue 00001(API キーの NVS 保存)
- WebSocket クライアントは links2004/WebSockets(yesman で実績あり)。TLS で接続、ヘッダ `Authorization: Bearer <key>` を付与
- モデルは `gpt-realtime`(GA)。イベントの組み立て/解釈は `src/pure/realtime_protocol.*` に分離し ArduinoJson でシリアライズ
- 接続状態(未接続 / 接続中 / セッション確立 / エラー)を画面に表示する簡易ステートマシン
- 事前にヒープ・PSRAM の空きをシリアルに出し、TLS + JSON バッファのサイズ問題に備える
