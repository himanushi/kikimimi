# 00013: セットアップポータルを http://kiki.mimi で開けるようにする

## What / Why

AP モードのセットアップで、ブラウザに IP でなく `http://kiki.mimi/` と表示・案内する。AP モード中は DNSServer がワイルドカードで全ドメインを SoftAP の IP に解決しているため、表示 URL・QR・captive リダイレクト先をドメインに変えるだけで動く。IP より覚えやすく「特別感」がある。

注意: これは AP モード限定。STA(通常の WiFi 接続中)ではルーターの DNS を握れないため kiki.mimi は使えない(00010 は IP の QR)。

## 受け入れ基準

- [ ] `portalStart()` が返す案内 URL が `http://kiki.mimi/` になる(検証: native テストまたはビルド + 実機画面の目視)
- [ ] AP に接続したスマホのブラウザで `http://kiki.mimi/` を開くと設定フォームが表示される(検証: 実機)
- [ ] captive portal 検出のリダイレクト先も `http://kiki.mimi/` になり、従来どおり設定ページに誘導される(検証: 実機でスマホ接続時の自動ポップアップ)
- [ ] 画面の URL QR のペイロードが `http://kiki.mimi/` になる(検証: 実機で QR を読んで開く)
- [ ] cores3 ビルドが通る(検証: `uvx --from platformio pio run -e cores3` SUCCESS)

## やらないこと

- STA モードでのドメイン対応(mDNS `kikimimi.local` は必要になったら別 issue)
- HTTPS 化(自己署名証明書の警告の方が体験を損なう)
- ポータルのフォーム・画面レイアウト変更

## How(実装方針)

- `src/portal.cpp` のみ: 定数 `PORTAL_DOMAIN = "kiki.mimi"` を追加し、`portalStart()` の返す URL と `handleNotFound()` の Location をドメインに変更。DNSServer は既にワイルドカード(`*`)なので変更不要
- main.cpp は `portalInfo.url` を表示しているだけなので触らない
- 注意: 一部 OS の captive portal ミニブラウザは検証用 URL 以外をブロックすることがある。実機で iOS / Android の両方を確認できない場合は確認できた側を result.md に記録する
