# 00014: 結果

## 検証の証跡

- `pure/vad.*`(エネルギー VAD): `test/test_vad/test_vad.cpp` を先に書き、実装なしでリンクエラー(red)を確認後、`src/pure/vad.cpp` を実装して green。ケース: 無音のみ / 発話→無音でハングオーバー後に終了 / 短いノイズでは開始しない / ハングオーバー中の発話再開でリセット / Ended は最終状態
- `pure/wav.*`(WAV ヘッダ): 同様に red → green。RIFF/WAVE マジック、16bit mono の各フィールド(sampleRate・byteRate・blockAlign・bitsPerSample)、データ長の反映を検証
- `pure/chat_protocol.*`(chat/completions の組み立て・解釈): 同様に red → green。system → 履歴 → 新規発話の順序、引用符エスケープ、`choices[0].message.content` と `usage`(`prompt_tokens_details.cached_tokens` の内数扱い)の抽出、壊れた JSON・`choices` 欠如時に `ok=false` になることを検証
- `pure/multipart_form.*`(STT アップロードの multipart 組み立て。plan.md には項目名がないが「How」の実装に必要で、デバイス非依存な純粋関数のためテスト対象に追加): 同様に red → green
- `usage_cost` 拡張(3 モデル単価): `calculateChatCostJpy` / `calculateSttCostJpy` / `estimateTtsCostJpy` を追加し、同様に red → green
- 自己検証: `uvx --from platformio pio test -e native` で 143 件全 green(既存 106 件 + 追加 37 件)。`uvx --from platformio pio run -e cores3` は SUCCESS(Flash 28.7%, RAM 16.0%)
- 実機確認が必要な受け入れ基準(未検証、実機確認待ち):
  - タップ → 発話 → 数秒で音声応答が再生され、会話テキストと累計金額が画面に表示される
  - 連続対話で文脈が維持される(「今の話を要約して」等)
  - エネルギー VAD の閾値(`VAD_CONFIG.startThreshold = 900`)が実際のマイク入力レベルに対して適切か(短すぎる/長すぎる発話区切りにならないか)

## 判断と経緯

- **`pure/multipart_form.*` を追加**: plan.md の受け入れ基準には明記されていないが、「How」に書かれた「multipart/form-data で WAV を送る」実装に必要で、境界文字列の組み立ては入出力が明確な純粋関数として切り出せた。CLAUDE.md の「テスト可能な部分はテストを先に書く」に従い、他の 3 モジュールと同様に red → green で実装した
- **STT レスポンス・エラーメッセージ抽出は pure モジュール化しなかった**: plan.md が pure 化を明示したのは vad/wav/chat_protocol の 3 つのみ。STT 応答の JSON(`{"text":..., "usage":...}`)は構造が単純なため `voice_pipeline.cpp` 内で直接 ArduinoJson を使い、スコープを広げなかった
- **HTTPS は証明書検証なし(`NetworkClientSecure::setInsecure()`)**: 前作 yesman の `http_helper.cpp`(issue 00053、オーナー合意済み)と同じ信頼モデル。kikimimi の `realtime_client.cpp` も `WebSocketsClient::beginSSL()` を CA 指定なしで使っており、既存の信頼モデルと整合させた
- **VAD 誤検知時のリカバリ**: STT の書き起こしが空文字になった場合(ノイズ等で VAD が誤って発話とみなした場合)、chat/TTS を呼ばずに `Listening` へ戻す。plan.md には明記されていないが、エネルギーベース VAD を採用した時点で避けられない誤検知シナリオであり、素通しで空発話をチャットに送るとおかしな応答になるため、この扱いを入れた
- **録音バッファが上限(15 秒)に達したときの扱い**: VAD が発話中と判定していれば、そこまでの録音を送信して打ち切る(強制終了)。ずっと無音・ノイズのままなら録音をリセットして聞き直す。plan.md の「開始直後から無音のままなら聞き直しへ」に対応する具体化
- **TTS の金額は概算**: `gpt-4o-mini-tts` は `/v1/audio/speech` のレスポンスに usage を返さないため、入力テキストの UTF-8 バイト数(日本語主体のため概ね 3 byte/token とみなす)と受信 PCM の再生時間(24kHz から逆算)× 概算音声トークンレート(20 tokens/秒、OpenAI の目安 $0.015/分 から逆算した値)で近似している。この概算値には検証手段がなく、実際の課金額とは乖離しうる
- **ADR 昇格**: 対話方式の変更は今後の対話関連 issue すべての前提になるため `docs/decisions/00004-stt-llm-tts-pipeline-instead-of-realtime-api.md` を追加した

## レビュー指摘と対応

フレッシュな subagent に diff と plan.md のみを渡してレビューを依頼した。

1. **TTS 受信タイムアウト時の fail-open(高信頼、対応済み)**: `readPcmResponse` がタイムアウトで受信ループを打ち切った場合でも、受信済みバイト数が 0 でなければ `callTtsApi` が成功扱いにしていた(途中で切れた PCM が無言で再生されうる)。`PcmReadResult` に `timedOut` を追加し、タイムアウトで打ち切った場合は明示的にエラー(`ErrorState` + `onError`)にするよう修正した(`src/voice_pipeline.cpp`)
2. **ADR・result.md の未作成の指摘**: レビュー時点ではまだ記録工程前だったための指摘。本ドキュメントと ADR 00004 の追加で対応した
