# 00013: 検証の証跡 / 判断と経緯 / レビュー指摘と対応

## 判断と経緯

- この issue は `portal.cpp` 内の 3 箇所(URL 文字列を組み立てる 2 箇所と `handleNotFound()` のリダイレクト先)を `WiFi.softAPIP().toString()` から定数 `PORTAL_DOMAIN` に差し替えるだけで、分岐や条件を持つロジックが存在しない。テスト対象になる純関数を切り出す余地がないため、TDD(red→green)は適用せず既存の native テストで回帰確認のみ行った。
- `PORTAL_DOMAIN` は AP モード限定であることをコメントで明示した(STA では DNSServer を握れないため無効)。plan.md 記載どおり STA/mDNS 対応・HTTPS 化・フォーム/画面レイアウト変更は行っていない。
- main.cpp は `portalInfo.url` を表示するだけの既存構造のため、`portal.cpp` の変更のみで画面表示・QR ペイロードの両方に反映される。main.cpp 自体は変更していない(00009 実装との競合回避のため、および plan.md の指示どおり)。

## 検証の証跡

- `uvx --from platformio pio test -e native`: 83 件全 PASSED(回帰確認、portal 関連のロジックテストを含む既存スイート)
- `uvx --from platformio pio run -e cores3`: SUCCESS(Flash 29.1%、RAM 15.9% 使用、警告なし)
- コードレビュー: フレッシュコンテキストの subagent に diff と plan.md のみを渡してレビュー。受け入れ基準適合・スコープ超過なし・秘密情報混入なし・コメントが why のみ・C++/String の扱いが既存コードと一貫、の 5 観点で「問題なし」の判定

## 未検証(実機確認待ち)

- AP に接続したスマホのブラウザで `http://kiki.mimi/` を開くと設定フォームが表示されること
- captive portal 検出時に自動ポップアップが `http://kiki.mimi/` へ誘導すること(iOS / Android 双方)
- 画面の URL QR を読み取ると `http://kiki.mimi/` が得られ、実際に開けること

## レビュー指摘と対応

- 指摘なし(「問題なし」の判定のみ)。対応不要。
