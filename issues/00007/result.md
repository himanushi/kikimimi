# 00007: 実装結果

## 検証の証跡

- `uvx --from platformio pio test -e native`: 63 件全て成功(新規 `test_usage_cost` 11 件、`test_realtime_protocol` への追加 2 件を含む)。usage のパース(`input_token_details` / `output_token_details` / `cached_tokens_details`)、金額計算(単価境界・ゼロ・全量 cached・部分 cached・複数種別合算)、円表示の整形(`¥0.0` / `¥12.3` / 四捨五入)をそれぞれ確認
- `uvx --from platformio pio run -e cores3`: ビルド成功(SUCCESS。RAM 15.8%, Flash 28.8%)
- 「会話画面に累計金額が `¥12.3` 形式で常時表示され、応答のたびに増える」「対話を終了して再度開始すると 0 から始まる」は**未検証(実機確認待ち)**。実機への upload はメイン側で行う

## 判断と経緯

- **単価の確認**: 実装前に WebSearch / WebFetch で gpt-realtime(コードにハードコードされているモデル名。`gpt-realtime-2` や `gpt-realtime-2.1` ではない)の公式 pricing を確認した。モデル詳細ページ `https://developers.openai.com/api/docs/models/gpt-realtime`(2026-07-13 に WebFetch で確認)に、GA モデルとして現在も有効(deprecated 表記や後継への案内なし)な状態で以下の価格が明記されている:
  - テキスト: input $4.00 / cached input $0.40 / output $16.00(1M トークンあたり)
  - 音声: input $32.00 / cached input $0.40 / output $64.00(1M トークンあたり)
  - plan.md の目安値と一致した。なお `https://openai.com/api/pricing/` は WebFetch で 403(既知の制約、`knowhow/realtime-api-ga-event-schema.md` にも記載あり)のため代替ソースを使用した
  - 補足: pricing 一覧ページ(`https://developers.openai.com/api/docs/pricing`)には `gpt-realtime-2.1` 系列のみが載っており無印 `gpt-realtime` の行は見当たらないが、その 2.1 の価格も audio 側は同額($32/$64)。モデル個別ページ(GA・現行と明記)を一次ソースとして採用した
- `src/pure/realtime_protocol.h` に `UsageTokens` 構造体を追加し、`ServerEvent` に `usage` フィールドとして持たせた。cached トークンは input のテキスト/音声トークン数の内数であり、別枠加算ではない旨をコメントで明記した
- 金額計算は新規 `src/pure/usage_cost.h/.cpp` に分離した(パース `realtime_protocol` と計算 `usage_cost` を別ファイルにし、受け入れ基準の 2 項目にそれぞれ対応させた)。非 cached 分は通常単価、cached 分は cached 単価で計算してから合算する方式にした(cached が内数のため、input トークン数そのものに通常単価をかけると過大計上になる)
- 表示整形 `formatJpyLabel()` も `usage_cost` の純関数としてテスト対象にした(`snprintf("%.1f")` で丸め・0 埋めを実機コードと切り離して検証できるようにするため)
- `realtime_client` はセッション累計 `sessionCostJpy` を保持し、`response.done` ごとに加算して `RealtimeCallbacks::onCostUpdated` で通知する。`realtimeConnect()` の先頭で 0 にリセットしてから `onCostUpdated(0.0)` を呼び、`main.cpp` 側の表示もセッション開始時点で確実に 0 に戻るようにした
- `main.cpp` は `drawRealtimeScreen()` の状態表示行に金額を併記する形にした(plan.md の「画面の状態表示行に併記する」に従い、新しい行は増やしていない)

## レビュー指摘と対応

フレッシュな subagent に diff(新規ファイル含む)と plan.md の受け入れ基準・やらないことのみを渡してレビューを実施(実装の経緯・自分の推論は渡していない)。

- **Medium**: `result.md` が存在せず、plan.md が求める「確認した単価と出典の記録」が確認できない → 対応済み(本ファイルの「判断と経緯」に記録)
- **Medium**: 採用した単価が plan.md の目安値と完全一致しており、独自に最新値を確認したか疑わしい → 実際に WebFetch でモデル詳細ページを取得し値を照合済み(結果的に目安値と一致した)。出典 URL と確認日を明記して解消
- コアロジック(usage パース、cached の内数処理、円換算、セッション累計とリセット)には高信頼の指摘なし。「やらないこと」(NVS 永続化・自動切断・動的為替)への抵触、新規パッケージ追加、個人情報・API キーの混入もいずれも指摘なし

## ADR 昇格判断

「単価をコード定数として持ち、為替は 160円/USD 固定」という方式は plan.md の「やらないこと」で明示的にスコープ外とされた動的取得の対極にある単純な実装判断であり、この issue 限りの決定(将来単価改定や永続化が必要になった時点で改めて設計判断が要る)。他 issue の前提になるプロジェクト共通の概念ではないため、ADR 昇格は不要と判断しこの result.md に留める。
