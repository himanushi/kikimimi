# 00002: 実装報告

## 検証の証跡

| 受け入れ基準 | 状態 | 証跡 |
|---|---|---|
| イベント生成・パースの純関数 + ホストテスト | ✅ | `uvx --from platformio pio test -e native` — 23 test cases: 23 succeeded(うち realtime_protocol 分 11 件)。スタブ実装で 9 件の red を確認してから実装(TDD) |
| ビルドが通る | ✅ | `uvx --from platformio pio run -e cores3` — SUCCESS(RAM 15.8%, Flash 28.1%) |
| タップで WebSocket 接続 → `session.created` がシリアルログで確認できる | ⬜ 未検証 | 実機書き込みが必要 |
| `session.update` 送信後に `session.updated` を受信する | ⬜ 未検証 | 同上。ペイロード仕様は WebSearch で GA(`gpt-realtime`)の `session.type=realtime` / `output_modalities` を確認して実装したが、実機での往復確認はできていない |
| テキスト送信 → `response.output_text.delta` を受信し画面表示 | ⬜ 未検証 | 同上 |
| 再タップ/切断でセッション終了、再タップで再接続 | ⬜ 未検証 | 同上 |
| API キー未登録・無効時にエラー表示 | ⬜ 未検証(未登録時の分岐はコードレビューで根拠確認済み) | 未登録は `realtimeConnect` 内でネットワーク接続を試みず即座にエラー表示する分岐を実装。無効キー(401 等)時の経路は実機でしか確認できない |

実機系 5 項目は未検証。書き込み後に確認し、このファイルのチェックを更新する。

## 判断と経緯

- **テキスト入力 UI は作らず、固定メッセージ(`GREETING_MESSAGE = "こんにちは"`)を `session.updated` 受信直後に自動送信する**: plan.md は「テキストの会話アイテム送信 → 応答受信」を受け入れ基準に含めるが、CoreS3 にはソフトキーボード等の文字入力手段がなく、00002 の目的は「接続・認証・イベントの往復の確認」に限定されている(plan.md の What/Why)。文字入力 UI の実装は 00002 のスコープ外と判断し、固定文言で往復を確認する構成にした
- **Realtime API のペイロードは GA(`gpt-realtime`)の新スキーマに合わせた**: `session.update` は `session.type="realtime"` / `output_modalities: ["text"]`、サーバイベント名は `response.output_text.delta` を使用(旧 beta 版の `modalities` / `response.text.delta` ではない)。WebSearch で `platform.openai.com/docs/api-reference/realtime-client-events` 系のドキュメントを確認して採用したが、実機での往復確認までは完了していない
- **TLS 証明書検証は yesman `push.cpp` の前例(fingerprint/CA 未指定で `setInsecure()` 相当)に倣った**: `beginSSL` にフィンガープリント・CA を渡していない。同一開発者の個人用デバイスという脅威モデルは 00001 の ADR(`docs/decisions/00002-api-key-provisioning-via-setup-portal.md`)と同じ前提であり、新たな ADR は起こさなかった
- **WStype_ERROR / WStype_DISCONNECTED を共通処理に統合**(レビュー指摘対応、詳細は下記): 当初 `WStype_ERROR` をログ出力のみで握りつぶしていたが、TLS ハンドシェイク失敗や認証エラーがどちらのイベントとして届くかライブラリ内部の挙動に依存するため、両方を `handleConnectionFailure()` に集約した
- **能動的切断とエラー切断を `disconnectRequested` フラグで区別**(レビュー指摘対応、詳細は下記): `ws.disconnect()` がコールバックを同期的に呼ぶ場合、意図的な切断が誤って「API キーエラー」として表示される問題を防ぐため

## レビュー指摘と対応

フレッシュコンテキストの subagent レビュー。指摘 5 件(high 2、medium 1、low 1、問題なし 1 カテゴリ)。high 2 件は対応、medium/low は記録のみで見送り:

1. **[high] `WStype_ERROR` の握りつぶし**(fail-open): TLS ハンドシェイク失敗・認証エラーが `WStype_ERROR` として届いた場合に画面が「接続中...」のまま固まる → `handleConnectionFailure()` に統合し、`WStype_DISCONNECTED` と同じ処理(状態遷移 + エラー表示)にした
2. **[high] `realtimeDisconnect()` と `onWsEvent(WStype_DISCONNECTED)` の順序依存**: `ws.disconnect()` がコールバックを同期的に呼ぶ実装だった場合、能動的切断が「API キーエラー」と誤表示される → `disconnectRequested` フラグを追加し、能動的切断中はエラー扱いしないようにした
3. **[medium] イベントコールバック内からの再入 `sendTXT`**: `session.created` 受信時や `Established` 遷移時のコールバック内から即座に `ws.sendTXT()` を呼んでいる。yesman `push.cpp` の `pushAck()` も同様のパターンで実運用されている前例があり、`links2004/WebSockets` が対応済みと判断して見送った。実機検証で問題が出れば送信をキューイングする対応に切り替える
4. **[medium] Realtime API ペイロード仕様が diff だけでは検証不能**: 実機検証(未検証項目)で確認する。plan.md の想定どおりの手順
5. **[low] 固定メッセージ自動送信は「テキストの会話アイテム送信」の簡易実装**: 上記「判断と経緯」に記載済み

## 関連

- 依存: issue 00001(API キーの NVS 保存)
- 次: issue 00003(音声送受信)。`realtime_client.h/.cpp` の接続・状態管理をそのまま拡張する想定
