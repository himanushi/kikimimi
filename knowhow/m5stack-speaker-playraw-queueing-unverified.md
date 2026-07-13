# M5Unified Speaker.playRaw() の連続呼び出しキューイング挙動は実機なしでは確認できない

- **症状**: `response.output_audio.delta` を受信するたびに `M5.Speaker.playRaw()` を都度呼んで逐次再生(真のストリーミング再生)したかったが、同一チャンネルへの連続呼び出しが「キューに追記される」のか「前の再生を打ち切って上書きする」のかが M5Unified のドキュメント・ソースだけでは断定できず、実機なしに検証する手段がなかった
- **対処**: issue 00003 では確実に動く設計を優先し、応答音声を PSRAM バッファへ全量貯めてから `response.done` 後に 1 回だけ `playRaw()` を呼ぶ方式にした(応答開始〜再生開始の遅延は増える)。判断の経緯は `docs/decisions/00003-realtime-audio-format-and-turn-detection.md`
- **次に踏むとしたら**: 真のストリーミング再生をやる場合は、まず実機で `playRaw()` を短い間隔で連続呼び出しし、`M5.Speaker.isPlaying()` やオシロ・録音で音切れ/上書きの有無を確認してから設計する
- **出典**: issue 00003(`m5stack-cores3/src/realtime_client.cpp` の `beginSpeaking()`)
