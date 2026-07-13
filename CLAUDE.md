# CLAUDE.md

## リポジトリ概要

**kikimimi** — Realtime API を使った音声対話 AI ロボット。M5Stack CoreS3 にビルドしたデバイスと会話し、会話から知識を蓄積していく「相棒」を作るプロジェクト。

前作 [yesman](https://github.com/himanushi)(目標達成マネジメントロボット)と違い、**目標設定や能動的な問いかけは行わない**。純粋な対話アプリ + 会話からの知識蓄積に絞る。

- **このリポジトリは public**。個人情報・メールアドレス・API キー・デバイス固有情報を含めないこと(コード・ドキュメント・コミットメッセージすべて)
- **動作指針**: @prompts/fable-behavior.md を毎セッション適用する(完了の証拠・ターン終了規律・報告スタイル)
- 開発の進め方: `docs/process/development-process.md`(必読)
- 決定の記録: `docs/decisions/`(ADR、決定のたび追記)
- docs/ の構成: `docs/README.md`(ナンバリングなし、役割別ディレクトリ)

## モノレポ構造(予定)

```
kikimimi/
├── m5stack-cores3/   ← 体(CoreS3 ファームウェア、Realtime API クライアント)
├── server/           ← 知識蓄積・Realtime API 中継(必要になったら)
└── docs/             ← 設計ドキュメント
```

## 作業ルール

- **issue 駆動**: 実装単位は `issues/NNNNN/plan.md`(作成は `/create-issue`、実装は `/implement-issue`)。実装は subagent(model: sonnet)へ background で委譲し、触るファイルが交差しない issue は並列実行する
- **knowhow の蓄積**: ハマった点は `knowhow/<カテゴリ>-<スラッグ>.md` に 1 件 1 ファイルで記録し、実装前に必読。M5Stack / PlatformIO 系は前作 `../yesman/knowhow/` の知見(`m5stack-*`, `pio-*`)も参照する
- **TDD**: テスト可能な部分はテストを先に書く
- コミットメッセージ: 日本語、prefix `add: / update: / fix: / refactor: / docs:`
- **commit / push は確認不要**: 作業の区切りごとに自動で commit・push する(ユーザーへの確認は不要)
- **日付・レビュー経緯の注記を書かない**: コード・ドキュメントに「(YYYY-MM-DD レビュー)」のような注記は残さない(履歴は commit で辿れる)。issue 番号の参照は可
- **コメントは why だけ**: コードが示せない制約・理由のみ書く(what/how の説明コメントは書かない)。1 箇所に閉じない構造的な why は ADR へ(使い分け: `docs/process/development-process.md`)

## 設計原則

- **秘密情報は repo に置かない**: API キーはサーバ側の secret またはビルド時の環境変数。デバイス・repo にハードコードしない
- **シークレットスキャン**: gitleaks pre-commit + CI + GitHub push protection で強制(`docs/decisions/00001`)。clone 後に `git config core.hooksPath .githooks`。`--no-verify` での hook バイパスは禁止。誤検知は `.gitleaks.toml` の allowlist に理由つきで追記
- **単純な対話に徹する**: リマインド・目標管理・能動的な問いかけ等の機能は作らない(前作との明確な差別化)
- **知識蓄積**: 会話から得た知識の保存・参照が中核機能。ストレージ方式は docs で決定してから実装する

## 永続化ルール

- 記憶はすべて repo 内ドキュメントに置く
- Claude Code の auto memory システムは使用しない(`~/.claude/projects/` への書き込み禁止)
