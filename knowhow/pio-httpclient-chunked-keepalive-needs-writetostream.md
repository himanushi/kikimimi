# HTTPClient で chunked + keep-alive のバイナリ応答は getStreamPtr の自前読みでは受けられない — writeToStream を使う

OpenAI の `/v1/audio/speech`(TTS)のようにレスポンスが Transfer-Encoding: chunked + keep-alive で返る場合、`http.getStreamPtr()` の生ストリームを自前ループで読む実装は 2 つの理由で壊れる:

1. **終端を検知できない**: keep-alive のため全データ受信後も `http.connected()` が true のまま。`available()==0` を待ち続け、必ずタイムアウトに落ちる(データは全部届いているのに「受信タイムアウト」エラーになる)
2. **フレーミング混入**: 生ストリームには chunked のサイズ行・CRLF が混ざるため、仮に読めてもバイナリ(PCM)が壊れる

## 対策

`HTTPClient::writeToStream(Stream*)` を使う。chunked のデコードと終端検知(サイズ 0 チャンク)を HTTPClient 側がやってくれる。バッファに受けたいだけなら、`write()` でバッファへコピーするだけの Stream サブクラスを渡す。戻り値は受信バイト数(負ならエラーコード)。

## なぜ重要か

kikimimi の TTS 受信(issue 00014)で「音声合成の受信がタイムアウトしました」として発症した。テキスト応答は `getString()`(内部で chunked 対応)なので問題にならず、バイナリを生ストリームで受けるときだけ踏む。
