# エネルギーベース VAD の閾値は実機のマイク入力レベルに依存し、事前に決め打てない

- **症状**: `pure/vad.*`(issue 00014)の `startThreshold`(発話ありとみなす振幅)は native テストでロジックを検証できるが、実際にどの値が M5Stack CoreS3 内蔵マイクの環境音・発話音量に対して適切かは実機なしに決められなかった
- **対処**: `src/voice_pipeline.cpp` の `VAD_CONFIG.startThreshold = 900` を仮値として設定した(int16 振幅の絶対値ピーク基準)。`minSpeechMs=200`, `hangoverMs=700` も同様に暫定値
- **次に踏むとしたら**: 実機で `M5.Mic.record` のピーク振幅をシリアル出力しながら、静かな環境・生活音のある環境それぞれで発話時/無音時の値を実測し、閾値を調整する。短い発話が切り捨てられる(閾値が高すぎる)・環境音で誤起動する(閾値が低すぎる)の両方を確認してから固定値にする
- **出典**: issue 00014(`m5stack-cores3/src/voice_pipeline.cpp` の `VAD_CONFIG`)
