# AI 生成コードの品質保証・検証プロセスの調査

AI が書いたコードをどう検証するかの調査。

---

## 1. なぜ専用の QA が必要か

- コミットされるコードの約 42% に AI が関与(Sonar 調査)。一方で AI 生成スニペットの 30〜40% に CWE クラスの脆弱性、論理・正確性バグは人手比 1.7 倍という報告
- 本質的な問題は「生成速度と品質保証速度のミスマッチ」。AI 特有の品質管理を追加しないと技術的負債が加速度的に蓄積する

## 2. AI コードレビュー

### レイヤード「AI ファースト」レビュー

1. PR オープン直後に AI がレビュー → 開発者が AI 指摘を先に解消
2. 人間レビュアーはアーキテクチャ・ビジネスロジック・UX に集中
3. SAST・カバレッジゲート・依存関係スキャンで多層化

### Adversarial verification(Builder / Critic 分離)

- 自己レビューは系統的に甘い(leniency bias)。生成とレビューが盲点を共有し「エコーチェンバー」になるため、**別セッション(または別モデル)の Critic** が仕様違反を根拠に却下を試みる構図が有効
- 運用の勘所: 指摘は高信頼度の上位数件に絞る(20 件羅列は無視される)
- 限界: LLM レビュアーの弱点はレースコンディション・タイミング・複雑な認可ロジック。SAST(Semgrep 等)の結果を LLM レビューに注入する併用が対策
- 出典: https://asdlc.io/patterns/adversarial-code-review/ / https://arxiv.org/html/2602.16741v1

## 3. テスト戦略

### トートロジーテストの罠

AI にコードとテストを両方書かせると、実装をなぞるだけの同語反復テストになり「実装が何をしていようと green」になる。カバレッジ 92% でも重大バグが素通りした実例あり。

### Mutation testing が「テストのテスト」

- 人工バグ(ミュータント)を注入し、テストが検出できる割合でテストスイートの実力を測る。カバレッジと無関係に弱さが露呈する
- 実践ループ: 生存ミュータント → 次のテスト生成プロンプトに戻す → 再実行で kill 確認。mutation ループを外すと欠陥検出率が 50% 低下(MuTAP 研究)
- 出典: https://blog.senko.net/improving-ai-generated-tests-using-mutation-testing

### AI との TDD

- 可能な限り red/green TDD(LLM が Red を飛ばして先に実装する挙動に注意)。機械的強制は hook で
- 反直感的な研究結果(TDAD): TDD プロンプトだけではむしろ回帰が増加。影響分析などの文脈情報の方が効く
- 出典: https://arxiv.org/html/2603.17973v1

### E2E 検証

- 実装後にエージェント自身が実機・実ブラウザで変更を動作検証させるのが新標準(構文的正しさだけでなく機能的検証まで)
- LLM 応答を含むシステムのテストは、決定的な状態検証+LLM 応答のモック(非決定性・コスト対策)が定石

## 4. プロンプト / LLM 出力の evals

- 基本ワークフロー: 人間がゴールデンデータセット(~200 件: 通常+エッジ+既知の失敗)を注釈 → LLM judge を人間ラベルとの一致率 85〜90% まで調整 → 以後 judge で継続監視
- judge 設計: Likert より**二値 pass/fail + 批評文**。最終判定の前に reasoning を書かせる。一致率でなくクラス別 precision/recall で測る
- プロンプト・データセット・grader をバージョン管理し CI に組み込み、ベースライン比較で回帰検出。ゴールデンデータは失敗例を追加し続ける「生きた資産」
- 出典: https://www.evidentlyai.com/llm-guide/llm-as-a-judge / https://huggingface.co/learn/cookbook/en/llm_judge

## 5. CI/CD への AI 組み込み

- Anthropic 公式: claude-code-security-review Action(diff 解析 → 文脈レビュー → 深刻度付き指摘 → 偽陽性フィルタ)、`/security-review`
- CI 内エージェント自体のリスク: Issue・PR diff 等の未信頼入力はプロンプトインジェクションを含むものとして扱う。外部からの PR は人間承認必須
- 出典: https://github.com/anthropics/claude-code-security-review / https://www.microsoft.com/en-us/security/blog/2026/06/05/securing-ci-cd-in-agentic-world-claude-code-github-action-case/

## 6. 供給網リスク: Slopsquatting

- LLM は実在しないパッケージ名を出力する(サンプルの約 20%、ハルシネーションの 43% は再現性があり攻撃者に予測可能)。偽パッケージを事前登録する攻撃が成立
- 対策: 新規依存は公式レジストリと正規名を照合、lock ファイル強制、CVE スキャン
- 出典: https://socket.dev/blog/slopsquatting-how-ai-hallucinations-are-fueling-a-new-class-of-supply-chain-attacks

## 7. 典型的な失敗パターンと対策

| 失敗パターン | 内容 | 対策 |
|---|---|---|
| Hallucinated API / パッケージ | 実在しない関数・ライブラリを自信満々に参照 | 型チェック・コンパイル・レジストリ照合 |
| 過剰実装・過剰抽象 | 防御的・抽象過多コードの反映 | スコープをプロンプトで明示的に制約、simplify レビュー |
| Reward hacking | 検証テストは通るが本質を解いていない(期待出力のハードコード等) | held-out テスト、仕様適合の独立検証 |
| Fail-open 挙動 | 例外を握りつぶして fallback を返す | エラーハンドリング専用の監査観点 |
| トートロジーテスト | 実装を写経したテスト | mutation testing、仕様由来のテスト |
| 破壊的操作の暴走 | 確認なしの削除・上書き | 権限最小化、確認ゲート、サンドボックス |

## 8. 推奨プロセス(総括)

1. 生成前: 仕様を明文化、スコープ制約をプロンプトに明記
2. 生成中: 型チェック / lint / コンパイルの即時フィードバック、依存の実在検証
3. 生成直後: フレッシュコンテキストによる adversarial レビュー(高信頼度指摘のみ)
4. テスト: TDD + 重要部の mutation score ゲート + エージェント自身による実機 E2E 検証
5. CI/CD: AI セキュリティレビュー、人間はアーキテクチャ・意図に集中
6. LLM 機能: ゴールデンデータ + LLM-as-judge を CI に組み込み継続更新
