# 00001: セットアップポータル(AP モードで WiFi と API キーを登録)

## What / Why

CoreS3 が未設定のとき SoftAP(`kikimimi-setup`)を立ち上げ、スマホ/PC のブラウザから WiFi の SSID・パスワードと OpenAI API キーを登録できる Web 設定画面を提供する。登録値は NVS に保存し、以後の起動では STA モードで自宅 WiFi に接続する。

API キーは 164 文字あり、デバイスのソフトキーボードで入力するのは非現実的。ブラウザからのコピペで登録できるようにするのが目的。これが後続の Realtime API 対話(00002, 00003)の前提になる。

## 受け入れ基準

- [ ] 設定の検証ロジック(SSID 非空・API キーの形式チェック等)が純関数で分離され、ホスト側テストが通る(検証: `pio test -e native`)
- [ ] ビルドが通る(検証: `pio run -e cores3` の出力)
- [ ] 未設定状態で起動すると SoftAP が立ち、画面に AP 名と設定用 URL が表示される(検証: 実機観察 + スマホから AP が見える)
- [ ] ブラウザの設定フォームから SSID / パスワード / API キーを送信すると NVS に保存され、その旨が画面に出る(検証: 実機観察)
- [ ] 再起動後、保存済み WiFi に STA 接続し、画面に接続状態(SSID・IP)が表示される(検証: 実機観察 + シリアルログ)
- [ ] WiFi 接続失敗時(パスワード誤り等)は AP モードに戻り再設定できる(検証: 実機でわざと誤パスワードを登録)

## やらないこと

- チャット履歴の Web 表示(将来 issue。設定用 Web サーバはその足場になる)
- 複数 WiFi の管理(保存は 1 件のみ。yesman の 8 件管理は持ち込まない)
- mDNS(`kikimimi.local`)での STA モード時アクセス(将来 issue)
- 設定画面の見た目の作り込み(素の HTML フォームでよい)
- API キーの暗号化保存(NVS 平文。帰結は result.md に記録し ADR 昇格を判断)

## How(実装方針)

- `m5stack-cores3/` を新規作成。platformio.ini は yesman のもの(pioarduino fork + M5Unified)をベースに、camera・WebSockets 等の不要依存を落とす
- WebServer(ESP32 Arduino 標準)+ DNSServer で captive portal 風に。フォーム 1 枚(SSID / パスワード / API キー)
- 保存は Preferences(NVS)。`config_store` モジュールに read/write を分離
- 入力検証(トリム・長さ・`sk-` プレフィックス等)は `src/pure/` の純関数にし、native 環境でテスト(yesman の `[env:native]` 構成を踏襲)
- 画面表示は M5GFX で状態テキストのみ(未設定 → AP 情報 / 設定済み → 接続状態)
- 実装前に `../yesman/knowhow/` の `m5stack-*`, `pio-*` を読む
