# 00007: 会話コストをリアルタイムに円換算で画面表示する

## What / Why

Realtime API は音声トークンが高価で、使った金額が見えないと安心して話せない。会話画面に「このセッションで使った金額(円)」を常時表示し、応答のたびに加算されていくようにする。

## 受け入れ基準

- [ ] `response.done` の usage(input/output のテキスト・音声・キャッシュ済みトークン数)のパースが純関数で分離され、ホストテストが通る(検証: `uvx --from platformio pio test -e native`)
- [ ] トークン数 → 金額(円)の計算が純関数で分離され、ホストテストが通る(検証: 同上。単価境界・ゼロ・キャッシュ分の扱いを含む)
- [ ] ビルドが通る(検証: `uvx --from platformio pio run -e cores3`)
- [ ] 会話画面に現在セッションの累計金額が「¥12.3」のような形式で常時表示され、応答のたびに増える(検証: 実機で数往復会話して観察)
- [ ] 対話を終了して再度開始すると金額が 0 から始まる(検証: 実機)

## やらないこと

- セッションをまたいだ累計の永続化(NVS 保存)
- 上限金額での自動切断・警告
- 為替レートの動的取得(160 円/USD の固定定数)

## How(実装方針)

- 単価は gpt-realtime の公式 pricing(1M トークンあたり USD)をコード定数にする。**実装前に WebSearch で https://openai.com/api/pricing/ 等から最新単価を確認し、確認した値と出典を result.md に記録する**(目安: audio input $32 / audio output $64 / text input $4 / text output $16 / cached input $0.40)
- `response.done` イベントの `response.usage`(`input_token_details` / `output_token_details`)を `src/pure/realtime_protocol.*` のパースに追加
- 金額計算は `src/pure/` の純関数(トークン内訳 → USD → 円。円は小数 1 桁表示)
- `realtime_client` がセッション累計を保持し、usage 更新のたびにコールバックで `main.cpp` へ通知 → 画面の状態表示行に併記する。セッション開始(realtimeConnect)でリセット
