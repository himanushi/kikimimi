# links2004/WebSockets: 接続失敗が WStype_ERROR と WStype_DISCONNECTED のどちらで来るか library 内部依存

- **症状**: TLS ハンドシェイク失敗・認証エラー(401 等)で接続が確立しないとき、`onEvent` コールバックに `WStype_ERROR` が来るか `WStype_DISCONNECTED` が来るかはライブラリ内部の実装・失敗の種類に依存し、片方だけを処理していると失敗時に画面が「接続中...」のまま固まる(無言の fail-open)
- **対処**: 両方のイベントを同じエラーハンドラに集約し、確立前(`Established` 未到達)の切断・エラーはまとめて「接続失敗」として扱う
- **関連**: `ws.disconnect()` を能動的に呼んだときにコールバックが同期的に発火する実装もあるため、能動的切断とエラー切断を区別するフラグ(例: `disconnectRequested`)を用意しないと、正常な切断が誤ってエラー表示される
- **出典**: issue 00002(`src/realtime_client.cpp` の `handleConnectionFailure()`)、レビュー指摘対応
