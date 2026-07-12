# Fable 5 と Opus 4.8 の差の分析(Opus/Sonnet への振る舞い移植)

Fable 5 がサブスクリプションから外れるため、Opus 4.8 / Sonnet で Fable 5 に近い性能を出す方法の調査。結論は `prompts/fable-behavior.md` に落とし込み済み。

---

## 1. 能力差の実態

| ベンチマーク | Fable 5 | Opus 4.8 |
|---|---|---|
| SWE-bench Verified | 95.0% | 88.6% |
| SWE-bench Pro | 80.0% | 69.2% |
| Terminal-Bench 2.0 | 84.3% | 74.6% |
| FrontierCode (Diamond) | 29.3% | 13.4% |

ただし差の出方には明確な構造がある:

- **タスクが長く・複雑・曖昧なほど差が開く**(Anthropic 公式も明言)。短く明確にスコープされたタスクではほぼ互角
- 917 共有シナリオの実測(tessl.io)では総合 92.9 vs 92.0 の **0.9 ポイント差**。2 ポイント閾値で Fable 勝ち 24% / Opus 勝ち 16%
- 19 回の A/B + 実務 26 セッションの検証(fablize)では、**答えの決まっている閉じた作業(コード・ロジック・ビルド)は実質互角**。差は open-ended な作業でのみ出た

つまり「モデルの地力の差」より「長時間・曖昧さへの耐性の差」が本体。タスクの切り方と運用で大部分は補える。

## 2. 差の分解: 移植できるもの / できないもの

### 移植できる(振る舞いの差 → プロンプトで転移)

Anthropic 公式の [Prompting Claude Fable 5](https://platform.claude.com/docs/en/build-with-claude/prompt-engineering/prompting-claude-fable-5) が Fable の振る舞いをプロンプトスニペットとして公開しており、コミュニティの移植実験(fablize / fable-method / skills 方式)でも転移が確認されている:

1. **証拠グラウンディング**: 進捗報告の各主張をツール結果と突き合わせる。Anthropic のテストでは捏造された進捗報告を「ほぼ排除」
2. **完了ゲート**: 検証(実行・観察)なしに「完了」と言わない。作った成果物は実際に動かして見る
3. **ターン終了規律**: 「次は X をやります」という約束で終わらず、そのまま実行する(早期停止防止)
4. **体系的調査**: 再現 → 仮説の競合 → 因果連鎖の追跡、という手順
5. **スコープ規律**: 頼まれていないリファクタ・抽象化・防御コードを足さない
6. **境界**: 問題の説明や質問には「評価」を返して止まる。修正は頼まれてから
7. **結論ファーストの報告**: 最初の一文が「何が起きたか」。矢印連鎖や省略記法でなく完全な文
8. **委譲**: 独立サブタスクは subagent に並列委譲(Opus はデフォルトで subagent を出し渋る)
9. **記憶**: 教訓を 1 件 1 ファイルで記録し参照する運用

### 移植できない(能力の差)

fablize の A/B で「指示やハーネスでは転移しなかった」と報告されたもの:

- **仕様外の欠陥発見**: チェックリストにない「何かおかしい」への気づき(注入実験で転移を否定)
- **含意の一歩先の深掘り**: 言われていないが論理的に続く一手を自発的に追う深さ
- **first-shot correctness**: 複雑な問題を一発で正しく実装する率

これらは運用で補う(§4)。

## 3. 移植先(Opus 4.8)側の特性

[Prompting Claude Opus 4.8](https://platform.claude.com/docs/en/build-with-claude/prompt-engineering/prompting-claude-opus-4-8) より。Fable と逆方向の調整が必要な点に注意:

- **effort は xhigh 推奨**(コーディング・エージェント用途)。Fable の `high` ≈ Opus の `xhigh` に相当。effort が歴代 Opus で最も効くモデル
- **規範的なスキャフォールドが効く**: Fable では手順を細かく指定すると出力が悪化するが、Opus では逆にステップバイステップの手順書が有効。振る舞い移植プロンプトが「手順を強制する」形式なのは Opus には適合
- **指示をリテラルに解釈**: 1 項目への指示を勝手に一般化しない。適用範囲は明示する
- **tool call よりも推論を好む**: 検証・検索を確実にやらせたい場合は明示指示 + effort 引き上げ
- **subagent を出し渋る**: いつ委譲すべきかの明示ガイダンスが必要
- xhigh 運用時は max output tokens を大きく(64k〜)

## 4. 能力差を運用で補う

移植不可の 3 項目への対策。いずれも本プロジェクトの開発プロセス(`docs/process/development-process.md`)と一致する:

| 埋まらない差 | 運用での補い方 |
|---|---|
| 長時間・曖昧タスクでの失速(差の本体) | **タスクを小さく切る**。issue 駆動・1 スライス 1 コミット。差は「短くスコープされたタスク」ではほぼ消える |
| first-shot correctness | 計画承認を挟む + failing test を先に書く + 自己検証ループで反復回数を確保 |
| 仕様外の欠陥への気づき | フレッシュコンテキストの adversarial レビュー、mutation testing、hook/CI での機械的検証 |
| 含意の一歩先 | 仕様(受け入れ基準)を書く段階で人間がエッジケースを列挙。AI にインタビューさせて掘る |

コミュニティの実測でも「スキル(手順書)の追加は約 17 ポイント改善、モデル変更は 1 ポイント未満」という報告があり、**モデル差よりワークフロー整備の方が寄与が大きい**。

## 5. 結論

- 短くスコープした閉じたタスクでは Opus 4.8(effort xhigh)は Fable とほぼ互角。差が出るのは長時間・曖昧・open-ended な条件
- 振る舞いの差(検証・証拠・完遂・報告・委譲)はプロンプトで移植可能 → `prompts/fable-behavior.md`
- 能力の差(気づき・深掘り・一発正答)はプロンプトでは埋まらず、タスク分割・レビュー分離・テスト先行の運用で補う → 既存の開発プロセスがそのまま対策になっている

## 出典

- https://platform.claude.com/docs/en/build-with-claude/prompt-engineering/prompting-claude-fable-5 (公式・Fable の振る舞いスニペット)
- https://platform.claude.com/docs/en/build-with-claude/prompt-engineering/prompting-claude-opus-4-8 (公式・Opus の調整)
- https://www.anthropic.com/news/claude-fable-5-mythos-5 (発表)
- https://github.com/fivetaku/fablize (A/B 検証: 転移可能/不可能の切り分け)
- https://github.com/Sahir619/fable-method (think/act/prove ループの蒸留、159 ラン eval)
- https://dev.to/toffy/want-to-keep-using-fable-5-teach-opus-and-sonnet-to-behave-like-it-4kl0 (skills 方式の移植)
- https://tessl.io/blog/claude-fable-5-vs-opus-48-the-mythos-hype-meets-reality/ (917 シナリオ実測)
- https://simonwillison.net/2026/Jun/9/claude-fable-5/ (使用感レビュー)
