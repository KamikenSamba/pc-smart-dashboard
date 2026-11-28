# PC Smart Dashboard 開発ガイド

## 役割

- Codexは既存コードの調査、設計案、実装、テスト、ドキュメント整備、GitHubでのIssue・PR管理を担当する。
- ユーザーは要件と開発方針を決め、実機操作、LCDの表示評価、PRの最終承認とマージ判断を担当する。
- 重要な設計変更、破壊的な操作、動作確認済みのハードウェア設定を変更する場合は事前にユーザーと合意する。

## GitHubとGit

- Issue → Branch → Implementation → Test → Commit → Pull Request → Review → Merge の順で進める。
- 着手前に既存のIssueとPRを確認して重複を避ける。原則として1 Issueにつき1つの機能と1つの作業ブランチを割り当てる。
- 最新の `origin/main` から作業ブランチを作り、`main` への変更はPR経由で行う。
- Issue・PRのタイトルは原則として英語とし、コミットは英語のConventional Commitsを使う。本文は日本語で書く。
- PR本文に `Closes #<Issue番号>` と、変更内容、テスト結果、残課題、該当する実機確認手順を記載する。
- マージにはユーザーの承認を得る。Squash mergeを基本とする。
- 既存の未コミット変更とstashを保持する。必要に応じて作業用worktreeを作る。force pushや破壊的なGit操作にはユーザーの明示的な承認を得る。

## 実装と確認

- 変更はIssueの範囲に絞り、既存の実装を再利用する。通信、計測、LCD描画の責務を明確に保つ。
- 動作中のPlatformIOボード設定、GPIO割り当て、LCDバスやドライバの設定を変更する前に、対象の基板と実機の構成を確認する。
- PC側の変更はリポジトリ直下から `.venv` のPythonで `python -m unittest discover -s pc/tests -v` を実行する。異常系と再接続への影響も確認する。
- ファームウェアに変更がある場合は、現行のPlatformIO環境でビルドする。提出前に差分と `git diff --check` を確認する。
- 自動テストとビルドの結果、ユーザーから報告を受けたWindows・ESP32での実機結果を区別して記録する。実機確認手順には操作、期待する動作、失敗時に確認するログを記載する。

開発状況と作業順序は [ロードマップ](docs/ROADMAP.md) を参照する。
