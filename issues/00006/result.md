# 00006: 実装結果

## 検証の証跡

- `uvx --from platformio pio test -e native`: 50 件全て成功(新規 `test_wifi_scan` 4 件を含む)。空 SSID 除外・RSSI 降順ソート・同名 SSID の重複除去(強い方を残す)をそれぞれ確認
- `uvx --from platformio pio run -e cores3`: ビルド成功(SUCCESS)
- 「設定ページを開くとプルダウンに一覧表示」「選択して保存 → 再起動 → 接続」「再スキャンで更新」「手入力欄の併存」は**未検証(実機確認待ち)**。実機への upload はメイン側で行う

## 判断と経緯

- スキャン結果の整形は `src/pure/wifi_scan.h/.cpp` の `dedupeSortWifiScan()` として純関数化した。yesman(`m5stack-cores3/src/net/wifi_pick.cpp`)の `dedupeSortScan` を参考にしたが、本 issue の受け入れ基準(重複除去・RSSI 降順・空 SSID 除外)のみに絞り、`saved` フラグや接続順決定などの yesman 側にある要素は持ち込んでいない(「やらないこと: 複数 WiFi の保存」と整合)
- SoftAP を止めないため `WiFi.mode(WIFI_AP_STA)` + `WiFi.scanNetworks(true)`(非同期)を `portalStart()` で開始。`handleRoot()` は `WiFi.scanComplete()` の戻り値(`WIFI_SCAN_RUNNING` / `WIFI_SCAN_FAILED` / 完了件数)を見て、未完了なら「スキャン中、再読み込みしてください」の案内を表示する
- 再スキャンは `/rescan` エンドポイントを新設し、`WiFi.scanNetworks(true)` を呼んで `/` へ 302 リダイレクトする方式にした。plan.md は「ページ再読み込みで新しい結果を反映」とだけ書かれていたが、明示的なトリガーがないと再スキャンが起きないため、リンクを 1 つ追加した(スコープ内の実装詳細と判断)
- select と手入力の優先順位は「手入力欄が空でなければ手入力を優先、空なら select の値を使う」というサーバ側ロジックのみで実装し、JS には依存していない
- **レビュー指摘への対応**: 初回実装ではバリデーションエラー再表示時に select の選択状態を保持していなかった。手入力欄の値だけが残るため、ユーザーが select で選び直しても手入力欄が空でない限りそちらが優先されてしまう不整合があった。`scanOptionsHtml()` / `scanSectionHtml()` / `formPage()` に選択中 SSID を引数として渡し、`selected` 属性で再表示時も select の選択を保持するよう修正した

## レビュー指摘と対応

フレッシュな subagent に diff と plan.md のみを渡してレビューを実施(実装の経緯は渡していない)。

- **Medium**: 手入力/プルダウンの優先順位が、バリデーションエラー再表示時に select の選択状態を保持していないため、ユーザーが select で選び直しても手入力欄の残留値が優先されてしまう → 対応済み(上記「判断と経緯」参照)
- **Low**: `scanSectionHtml()`(描画用の関数)内で `WiFi.scanNetworks(true)` の副作用が起きるのは命名と責務が一致していない → 動作上は自己修復的で受け入れ基準を侵さないため、コメントで意図を明記したまま据え置き(スコープ外の追加リファクタは避けた)
- **Low**: エラー再表示時に select の選択状態が保持されない → Medium 対応と合わせて解消済み

## ADR 昇格判断

「AP_STA + 非同期スキャンで SoftAP を止めない」「select と手入力の優先順位をサーバ側ロジックのみで解決する」は、いずれも WiFi 設定ポータルというこの機能領域固有の判断であり、他 issue の前提になるプロジェクト共通の概念ではない。ADR 昇格は不要と判断し、この result.md に留める。
