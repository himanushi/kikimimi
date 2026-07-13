# 00011: 複数 WiFi の登録と電波強度による自動選択

## What / Why

WiFi を 1 件しか保存できないため、自宅・外出先など場所が変わるたびに再設定が必要。複数の WiFi(SSID + パスワード)を登録しておき、起動時にスキャンして「登録済みのうち電波(RSSI)が最も強いもの」から順に接続を試みる。

依存: 00010(設定ページ STA 常設)。

## 受け入れ基準

- [ ] 保存済み WiFi リストとスキャン結果から接続候補を RSSI 降順で返す純関数がテストされる(検証: `uvx --from platformio pio test -e native` green。先に red。ケース: 複数一致 → RSSI 降順 / 一致なし → 空 / 同一 SSID が複数 AP で見える → 最強の 1 件)
- [ ] WiFi は最大 5 件登録でき、NVS に保存される(検証: native テスト + 実機で 2 件登録)
- [ ] 起動時、登録済み WiFi のうち最強のものに接続する。失敗したら次の候補に順に試行し、全滅なら AP ポータル(検証: 実機で 2 件登録し、片方の AP を切って挙動確認)
- [ ] 設定ページに登録済み WiFi の一覧(SSID のみ。パスワードは非表示)と追加・削除がある(検証: 実機のブラウザ操作)
- [ ] 既存の 1 件保存からの移行: 旧キーで保存済みの SSID/pass はリストの 1 件として引き継がれる(検証: native テストまたは実機で更新前の設定が消えないこと)
- [ ] cores3 ビルドが通る(検証: `uvx --from platformio pio run -e cores3` SUCCESS)

## やらないこと

- 接続中のローミング(より強い AP への自動切替)。選択は起動時のみ
- 優先度の手動指定(常に RSSI 順)
- 5 件超の登録

## How(実装方針)

- `src/config_store.*`: WiFi リスト(最大 5 件)の保存・読込。ArduinoJson で JSON 化して NVS に 1 キーで保存すると移行・拡張が楽。旧 `ssid`/`pass` キーが存在すれば読込時にリストへ移行
- `src/pure/wifi_select.h/.cpp`(新規): `selectWifiCandidates(saved, scanResults)` → RSSI 降順の候補列。native でテスト
- `src/main.cpp`: 起動時 `WiFi.scanNetworks()` → 候補列に沿って順に接続試行(タイムアウトは現行 15 秒を候補ごとに短縮調整可)
- `src/portal.*`: フォームに登録済み一覧 + 削除ボタン、追加フォーム(スキャン結果プルダウンは現行流用)。API キーは従来どおり単一
- `src/pure/setup_config.*`: バリデーションは現行を流用(SSID/pass の制約は同じ)
