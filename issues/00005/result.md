# 00005: 実装結果

## 検証の証跡

- 表示状態の判定を `src/pure/portal_screen.h/.cpp`(`portalScreenForStationCount(int stationCount)`)に純関数として分離し、`test/test_portal_screen/test_portal_screen.cpp` で先にテストを書いた。実装前は未定義参照でリンクエラー(red)になることを確認し、最小実装後に `uvx --from platformio pio test -e native` で全 46 ケース PASSED を確認した(既存 4 スイートも含めて green)。
- `uvx --from platformio pio run -e cores3` で SUCCESS を確認した(Flash 28.7%, RAM 15.8%)。
- 実機 + スマホでの確認(「AP 接続で URL 案内に切り替わる」「URL QR でブラウザが開く」「切断で AP 接続 QR に戻る」)は**未検証(実機確認待ち)**。upload はメイン側で行うため本 issue のスコープでは実施していない。

## 判断と経緯

- `portal.cpp` に `portalStationCount()`(`WiFi.softAPgetStationNum()` のラッパー)を追加し、`main.cpp` 側の `loop()` で毎回ポーリングして `updatePortalScreen()` に渡す構成にした。plan.md の How に記載された方針どおり。
- `main.cpp` の描画関数を `drawPortalScreen` から `drawApJoinScreen` / `drawUrlGuideScreen` の 2 つに分割した。既存の「テキスト行(左)+ QR(右、画面右端寄せ)」というレイアウトパターンをそのまま踏襲し、新規のレイアウト設計はしていない。
- 直前に描画した接続台数(`lastPortalStationCount`)を保持し、値が変化したときだけ再描画する。`M5.Display.qrcode()` の毎ループ呼び出しは画面のちらつき・処理コストの点で不要なため。
- URL QR のペイロードは `info.url`(`http://192.168.4.1/` 相当)をそのまま渡している。API キー等の秘密情報は乗らない。00004 で「設定 URL 側は QR 化しない」としていた制約は、plan.md に記載の通り本 issue で解除する(ユーザー確認済み)。

## レビュー指摘と対応

フレッシュな subagent に diff と plan.md のみを渡してレビューを実施した。

- medium 指摘 1 件(グローバル変数の初期化順序への懸念)はレビュー自身が診断の過程で誤りと判明し撤回。対応不要。
- medium 指摘 1 件(`loop()` 内で毎回 `portalStationCount()` を呼ぶ点)は plan.md の How に明記された方針どおりであり、性能上の軽微な指摘に留まったため対応不要と判断。
- low 指摘・確認事項(テストの実質性、スコープ順守、秘密情報混入なし)はすべて問題なしの確認で、対応不要。
- high 指摘はなし。

## ADR 昇格判断

「接続台数のポーリングで画面を切り替える」「QR ペイロードは URL 文字列そのまま」は本 issue 固有の実装判断であり、他の issue の前提となる方式決定(ツール選定・アーキテクチャ)ではない。ADR は追記しない。
