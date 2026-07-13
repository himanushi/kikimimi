# 00010: 設定ページの STA 常設 — 実装結果

## 検証の証跡

- 判定純関数 `pure/settings_entry.{h,cpp}`(`settingsEntryAction(bool wifiConnected)`)を先にテストのみ追加し、`uvx --from platformio pio test -e native -f test_settings_entry` で red(ヘッダ未実装によるビルドエラー)を確認してから実装した
- `uvx --from platformio pio test -e native`: 95 件全 green(既存 93 件 + 新規 2 件)
- `uvx --from platformio pio run -e cores3`: SUCCESS
- 受け入れ基準:
  - [x] 判定純関数がテストされる(上記)
  - [ ] WiFi 接続済みで設定ボタン → STA の URL QR 表示(実機未検証)
  - [ ] STA 側の設定ページで SSID / パスワード / API キーを保存 → 再起動(実機未検証)
  - [ ] QR 表示画面からタップで戻る(実機未検証。コード上は `mode = Mode::SETTINGS_URL_GUIDE` でタップ時に `Mode::SETTINGS` へ戻る実装)
  - [ ] WiFi 未設定・接続失敗時は従来の AP ポータル(実機未検証。既存コードパス `startPortal` は無変更)
  - [x] cores3 ビルド SUCCESS

**実機で確認すべき項目(未検証)**:
- 設定ボタンタップ時にスマホの WiFi 接続先が AP に切り替わらないこと
- STA の IP QR を読み取って設定ページが開けること、フォームで SSID/パスワード/API キーを保存でき再起動後に新設定で接続すること
- QR 画面をタップで閉じて設定画面に戻れること
- QR 画面を閉じた後も同じ URL でフォーム送信(`/save`)が正常に処理されること(下記「判断と経緯」参照)

## 判断と経緯

- **STA 用 Web サーバの起動タイミングを「タップ時」から「起動時(setup 内、WiFi 接続成功直後)」に変更した**(plan.md は「設定ボタン → STA なら URL QR 画面を表示」としていたが、レビューで指摘された不具合を踏まえて変更)。理由: 当初はタップ時に `portalStartSta()` を呼び、`mode == SETTINGS_URL_GUIDE` のときだけ `portalLoop()`(`webServer.handleClient()` を含む)を呼ぶ実装にしたところ、QR 画面をタップで閉じた瞬間に `webServer.handleClient()` を呼ぶ経路がなくなり、スマホ側でフォーム送信中でも応答不能になる不具合があった(フレッシュ subagent のレビューで指摘)。「STA 常設」というタイトル・plan.md の「Web サーバは STA 側で稼働」という表現からも、画面状態に関わらず稼働し続けるべきと判断し、起動時に一度だけ Web サーバを起動して以後 `mode != Mode::PORTAL` の間は毎ループ `portalLoop()` を呼び続ける設計に変更した。QR 画面(`SETTINGS_URL_GUIDE`)固有の役割は「URL 案内を表示し、タップで設定画面に戻る」だけに縮小した
- `portal.cpp` の WebServer 起動処理を `registerFormHandlers()` として AP・STA で共通化し、captive DNS(`dnsServer`)・接続台数(`portalStationCount()`)・404 の `kiki.mimi` リダイレクトは `apActive` フラグで AP のときだけに限定した。STA では `kiki.mimi` が解決できないため、404 時は相対パス `/` へリダイレクトするようにした
- API キーの現在値をフォームに表示しない既存方針は `formPage`/`handleRoot` を AP・STA で共有しているため変更なく維持されている

## レビュー指摘と対応

フレッシュな subagent に diff と plan.md のみを渡してレビューさせた。

1. **[対応済み] STA の Web サーバが QR 画面を閉じると応答不能になる(高信頼)**: `portalLoop()`(`webServer.handleClient()` を含む)が `Mode::SETTINGS_URL_GUIDE` のときにしか呼ばれず、タップで `Mode::SETTINGS` に戻った後は `/save` への POST がハングする指摘。上記「判断と経緯」のとおり、STA 用 Web サーバを起動時に一度だけ起動し、`mode != Mode::PORTAL` の間は常時 `portalLoop()` を呼ぶよう設計変更して解消した
2. **[対応済み] STA ガイドへの再入場でハンドラが重複登録される(中信頼)**: `portalStartSta()` をタップのたびに呼んでいたことによる `webServer.on(...)` の重複登録の指摘。起動時に一度だけ呼ぶ設計変更により、再入場時は `enterStaUrlGuide()` が画面描画のみ行う(`portalStartSta()` を呼ばない)ため解消した
3. 良好な点として指摘された、API キー非表示方針の維持・STA での `kiki.mimi` 未使用は変更なし

## ADR 昇格判断

「STA 常設時は Web サーバを起動時から待ち受けさせ、画面状態と Web サーバ稼働を分離する」という判断は、この issue 固有の設計判断であり、今後の issue の前提になるプロジェクト共通の概念(ツール選定・構成方式)ではないため ADR には昇格しない。result.md に留める。
