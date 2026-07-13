# M5Unified Speaker.setVolume() は begin() 後に呼ぶ必要があるかが実機なしでは断定できない

- **症状**: 設定画面での音量変更を、対話再開時(`beginSpeaking()` での `M5.Speaker.begin()` 直後)にも反映させたかったが、`setVolume()` を `begin()` 前に呼んだ場合に値が保持されるのか、`begin()` で音量設定がリセットされるのかがドキュメント・ソースだけでは断定できなかった
- **対処**: 安全側に倒し、`M5.Speaker.begin()` の直後に毎回 `M5.Speaker.setVolume(speakerVolume)` を呼ぶ設計にした(issue 00012、`m5stack-cores3/src/realtime_client.cpp` の `beginSpeaking()`)
- **次に踏むとしたら**: 実機で `begin()` → `setVolume()` の順を入れ替えて音量が反映されるかを確認する。反映されないパターンがあれば、`begin()` 前に設定しても無視される/上書きされるケースがないか確認してから設計を見直す
- **出典**: issue 00012
