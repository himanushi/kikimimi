# links2004/WebSockets は受信フレーム 15KB 超で即切断する(Realtime API の音声 delta が超える)

- **症状**: Realtime API で発話終了(speech_stopped)直後、応答イベントが届く前に `ws disconnected`。エラーイベントは来ず、ライブラリの自動再接続で session.created からやり直しになる
- **原因**: `WebSockets.h` の `WEBSOCKETS_MAX_DATA_SIZE` が ESP32 で 15KB 固定。`response.output_audio.delta` のフレームはこれを超えるため、ヘッダの payloadLen を見た時点で close 1009 を送って切断される(payload は読まれないので何のイベントかもログに出ない)
- **対処**: `#ifndef` ガードがなく build_flags の `-D` では上書きできない(ヘッダ側の #define が勝つ)。`extra_scripts = pre:scripts/patch_websockets_max_size.py` で libdeps 展開後のヘッダを 256KB に書き換える。大きな受信 payload の `malloc` は SPIRAM_USE_MALLOC で PSRAM に逃げるため内部 RAM は枯渇しない
- **教訓**: WebSocket ライブラリの「切断」はネットワーク起因とは限らない。フレームサイズ上限・malloc 失敗でもクライアント側から切る実装がある。切断の再現タイミングが「大きなデータが届き始める瞬間」なら受信バッファ制限を疑う
- **出典**: issue 00003 の実機検証(kikimimi)。`.pio/libdeps/cores3/WebSockets/src/WebSockets.cpp` の `handleWebsocket`(payloadLen チェックと close 1009)
