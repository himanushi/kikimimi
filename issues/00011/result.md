# 00011: 複数 WiFi の登録と電波強度による自動選択 — 実装結果

## 検証の証跡

- 選択純関数 `pure/wifi_select.{h,cpp}` と JSON 化・移行・追加/削除純関数 `pure/wifi_list.{h,cpp}` を先にテストのみ追加し、
  `uvx --from platformio pio test -e native -f test_wifi_list -f test_wifi_select` でリンクエラー(未実装)による red を確認してから実装した
- `uvx --from platformio pio test -e native`: 111 件全 green(既存 95 件 + 新規 16 件)
- `uvx --from platformio pio run -e cores3`: SUCCESS
- 受け入れ基準:
  - [x] 保存済み WiFi リストとスキャン結果から接続候補を RSSI 降順で返す純関数(`selectWifiCandidates`)。ケース(複数一致 / 一致なし / 同一 SSID 複数 AP)を native テストで検証
  - [x] WiFi は最大 5 件登録でき NVS に保存される(`addOrUpdateWifiCredential` の上限テスト + `configSave`/`configLoad` での JSON 保存)。実機での 2 件登録は未検証(実機確認待ち)
  - [ ] 起動時、登録済み WiFi のうち最強のものに接続、失敗したら次候補、全滅なら AP ポータル(`connectToBestWifi` としてコードで実装。実機での挙動確認は未検証(実機確認待ち))
  - [ ] 設定ページに登録済み一覧(SSID のみ)+ 追加・削除がある(`portal.cpp` にフォーム実装済み。実機のブラウザ操作は未検証(実機確認待ち))
  - [x] 既存の 1 件保存からの移行(`migrateLegacyWifi` を native テストで検証。実機での移行確認は未検証(実機確認待ち))
  - [x] cores3 ビルド SUCCESS

**実機で確認すべき項目(未検証)**:
- WiFi を 2 件登録し、片方の AP の電源を切った状態で起動して、もう片方(または RSSI が強い方)に自動接続すること
- 登録済み一覧に SSID のみ表示され、パスワードが表示されないこと
- 一覧からの削除がその場で反映され(再起動なしで一覧から消え)、再起動しても復活しないこと(下記「判断と経緯」のバグ修正箇所)
- 旧バージョン(単一 ssid/pass 保存)からアップデートしたデバイスで、起動後の設定ページに既存の WiFi が 1 件目として表示されること
- 6 件目の WiFi 登録を試みるとエラーメッセージが表示され、登録されないこと

## 判断と経緯

- **WiFi リストの保存形式**: `config_store.h` の `StoredConfig` を `ssid`/`pass` 単一フィールドから `std::vector<WifiCredential> wifiList` に変更し、`configSave()` のシグネチャも `(wifiList, apiKey)` に変更した。plan.md の「ArduinoJson で JSON 化して NVS に 1 キーで保存」の方針どおり
- **API キーの扱い**: plan.md の「API キーは従来どおり単一」に従い、WiFi の追加フォームには毎回 API キー入力欄を残した(既存の `validateSetupInput` を無変更で流用)。WiFi を追加するたびに API キーの再入力が必要になるが、plan の「バリデーションは現行を流用」を文字どおり適用した結果であり、UX 改善は今回のスコープ外と判断した
- **削除操作は再起動しない**: `/save`(追加)は従来どおり再起動を伴うが、`/wifi/delete` は接続中のセッションを不要に切断しないよう、リスト更新後に一覧ページへ 302 リダイレクトするだけで再起動はしない設計にした。plan.md はこの点を明示していないが、「削除は次回起動時の接続候補が減るだけ」という受け入れ基準の趣旨から妥当と判断した
- **CSRF 対策は追加しない**: 削除ボタンは POST フォームのみで CSRF トークンは付けていない。既存の `/save` エンドポイントも同様の設計であり、レビューでも「一貫性があり過剰防御・過小防御のどちらでもない」と確認された

## レビュー指摘と対応

フレッシュな subagent に diff(`git diff --cached`)と plan.md のみを渡してレビューさせた。

1. **[対応済み・高信頼] 移行済み WiFi を削除しても再起動で復活する**: `configLoad()` は毎回 NVS の旧キー `ssid`/`pass` を読み、リストに未登場なら `migrateLegacyWifi()` で追加する。しかし移行後も旧キーを消していなかったため、ユーザーが移行された WiFi を削除して保存しても、次回起動時に旧キーから再度追加されてしまうバグがあった。`configSave()` で `Preferences::remove("ssid")`/`remove("pass")` を呼び、保存のたびに旧キーを消去するよう修正した。詳細は `knowhow/pio-nvs-legacy-key-migration-must-clear-old-keys.md` に記録した
2. **[対応済み・軽微] `/wifi/delete` が保存失敗を無視していた**: `handleSave()` は `configSave()` の失敗時に 500 とエラーメッセージを返すのに対し、`handleDeleteWifi()` は戻り値を無視して常に 302 リダイレクトしていた。`handleSave()` と同じパターンでエラー応答を返すよう修正した
3. 良好な点として、`selectWifiCandidates` のテストが振る舞い(RSSI 降順・一致なし・同一 SSID 複数 AP)を実質的に検証していること、5 件上限の強制、SSID のみ表示・エスケープ、CSRF トークン無しの一貫性が確認された

## ADR 昇格判断

「WiFi リストを JSON 化して NVS 1 キーに保存し、旧キーは移行後に消去する」という判断はこの issue 固有の実装詳細であり、今後の issue の前提になるプロジェクト共通の概念(ツール選定・構成方式)ではないため ADR には昇格しない。result.md と knowhow に留める。
