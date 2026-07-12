# 00001: シークレット・個人情報の混入防止を多層ガードレールで強制する

## 文脈

このリポジトリは public であり、API キー・個人情報・デバイス固有情報を含めないルールがある(CLAUDE.md)。しかし CLAUDE.md の記述は「助言」であり強制力がない。AI 駆動開発では、エージェントが `.env` を読んでコードに埋め込む・pre-commit hook を `--no-verify` でバイパスする、といった漏洩経路が実際に報告されている。ルールは機械的に強制する必要がある(文脈と強制の区別、`docs/analysis/ai-context-management.md`)。

## 決定

シークレットスキャンの定番構成(gitleaks を commit/CI ゲートに、プラットフォーム層に GitHub secret scanning)を採用し、4 層で強制する:

1. **pre-commit(gitleaks)**: `.githooks/pre-commit` がステージ済み差分をスキャンしてブロック。`.gitleaks.toml` でデフォルト 150+ ルールに加え、個人プロバイダのメールアドレス(gmail 等)検出を追加(public repo 固有の要件)
2. **CI(gitleaks-action)**: `--no-verify` でローカルをすり抜けた場合の二層目。push / PR 時に全履歴スキャン
3. **Claude Code ガード(`.claude/settings.json`)**: `.env` / `*.pem` / `*.key` / `~/.ssh` 等の読み取りを deny。PreToolUse hook(`.githooks/block-git-bypass.sh`)で `--no-verify` / `--no-gpg-sign` / `core.hooksPath` 付け替えを拒否
4. **GitHub push protection**: リポジトリ設定で secret scanning + push protection を有効化済み

比較検討: TruffleHog は実弾検証(API 照合)に強いが pre-commit にはネットワーク依存で不向き。detect-secrets はレガシー repo のベースライン用。pre-commit には高速・オフラインの gitleaks が定石。

## 帰結

- clone 後のセットアップとして `git config core.hooksPath .githooks` が必要(CLAUDE.md に記載)
- pre-commit が誤検知した場合は `.gitleaks.toml` の allowlist に理由つきで追記する。`--no-verify` での回避は禁止
- 実際に漏れた場合の対応は「まずキーのローテーション」。履歴からの削除だけではキーは無効化されない
- deny 設定・hook はソフトな防御であり、根本対策は「秘密情報を平文でリポジトリ配下に置かない」こと(サーバ側 secret / ビルド時環境変数)
