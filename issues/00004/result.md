# 00004: 結果

## 検証の証跡

- WiFi QR ペイロード生成(`src/pure/wifi_qr.*`): `uvx --from platformio pio test -e native` で `test_wifi_qr` を含む 31 テストケースすべて PASSED。エスケープ対象(`\ ; , " :`)ごとの単独ケースと複合ケース、オープンネットワーク(`P:` 省略)を確認済み
- ビルド: `uvx --from platformio pio run -e cores3` が SUCCESS(Flash 28.2%、RAM 15.8%)
- 画面表示・スマホでの QR 読み取り接続: **未検証(実機書き込み待ち)**。plan.md の受け入れ基準のうち実機観察・実機+スマホの 2 項目は本セッションでは確認できていない

## 判断と経緯

- `M5.Display.qrcode()` のシグネチャは推測せず、`.pio/libdeps/cores3/M5GFX/src/lgfx/v1/LGFXBase.hpp` のローカルキャッシュと GitHub 上の `lovyan03/LovyanGFX` 最新版の両方を突き合わせて確認した(`void qrcode(const char*, x=-1, y=-1, width=-1, version=1, margin=false)`)
- 画面レイアウトは「左: テキスト情報 / 右: QR」(320x240 の CoreS3 画面)。plan.md の「画面レイアウトの作り込みはしない」に従い、座標は最小限の固定値にとどめた
- 既存の `drawLines()` はレンジベース for での `{...}` 初期化子リストが `String` と `const char*` の混在で型推論に失敗したため、当初の実装ミスを修正して既存の `drawLines()` をそのまま再利用する形にした(新規のテキスト描画ロジックを増やさない)。詳細は `knowhow/m5stack-string-literal-initializer-list-type-mismatch.md` 参照

## レビュー指摘と対応

フレッシュな subagent に diff と plan.md のみを渡してレビューを実施した。

- **Medium: テキストと QR の重なりリスク**(初期実装 `x=190, y=60, w=120`)— テキスト行の右端と QR 左端の間にマージンがなく、行の長さによっては QR に文字が接触しうるとの指摘。QR の位置を `x=210, y=50, w=100` に変更し、テキスト起点(x=12)からのマージンを確保した。実機上での最終確認は未実施
- High 指摘なし。ペイロードのエスケープ実装は WIFI QR の標準フォーマット(ZXing 由来)と整合していると確認された

## 未検証項目(次のアクション)

- 実機書き込み後、ポータル画面で QR とテキストが重ならず表示されることの目視確認
- スマホのカメラで QR を読み取り、AP(`kikimimi-setup`)への接続が提案・成功することの確認

## ADR 昇格判断

今回の判断(QR ペイロードの純関数分離、`M5.Display.qrcode` の使用、テキスト/QR 分割レイアウト)はいずれも issue 00004 限りの実装詳細であり、今後の issue の前提となるプロジェクト共通の方式選定ではない。ADR 昇格は不要と判断した。
