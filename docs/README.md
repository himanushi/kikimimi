# docs

ナンバリングはせず、役割ごとのディレクトリで分ける。

| ディレクトリ | 役割 |
|---|---|
| `analysis/` | 調査レポート(技術・手法の分析) |
| `process/` | 開発の進め方(プロセス・運用ルール) |
| `specs/` | 要件・設計(作成予定) |
| `decisions/` | ADR: 決定の記録 |

## 現在のドキュメント

- [process/development-process.md](process/development-process.md) — このプロジェクトの開発手順(まずこれを読む)
- [analysis/ai-driven-development-workflow.md](analysis/ai-driven-development-workflow.md) — AI 駆動開発ワークフローの調査
- [analysis/ai-context-management.md](analysis/ai-context-management.md) — コンテキスト管理・知識蓄積の調査
- [analysis/ai-code-quality-assurance.md](analysis/ai-code-quality-assurance.md) — AI 生成コードの品質保証の調査
- [analysis/fable-vs-opus-analysis.md](analysis/fable-vs-opus-analysis.md) — Fable 5 と Opus 4.8 の差の分析(振る舞い移植の根拠)
- [decisions/00001-secret-scanning-guardrails.md](decisions/00001-secret-scanning-guardrails.md) — シークレット混入防止の多層ガードレール
- [decisions/00002-api-key-provisioning-via-setup-portal.md](decisions/00002-api-key-provisioning-via-setup-portal.md) — API キーはセットアップポータルで登録し NVS 保存
- [decisions/00003-realtime-audio-format-and-turn-detection.md](decisions/00003-realtime-audio-format-and-turn-detection.md) — 音声は PCM16 24kHz + semantic_vad、ターン全量受信後に再生
