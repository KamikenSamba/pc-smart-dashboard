# PC Smart Dashboard ロードマップ

## 現在の開発状況

- PC側はWindows上のPythonでCPU・RAM使用率を取得し、115200 bpsのUSBシリアル通信で改行区切りJSONを送る。
- ESP32はArduinoJsonで受信値を解析し、TFT_eSPIで480×320のLCDにCPU・RAMの数値とバーを描く。値が届いたときに該当領域を部分更新する。
- 基本監視の送信、LCD更新、Ctrl+C終了は実機確認済み。USB切断や未接続時に再試行する機能は、Windows・ESP32でユーザーが動作を確認し、[PR #18](https://github.com/KamikenSamba/pc-smart-dashboard/pull/18)で `main` に取り込んだ。[基本監視の実装記録](https://github.com/KamikenSamba/pc-smart-dashboard/pull/16)
- `main` の再接続は同じCOM番号を使用する。COM番号が変化したときにも復帰できるよう、[Issue #20](https://github.com/KamikenSamba/pc-smart-dashboard/issues/20)で自動検出と手動ポート指定を扱う。

## 基板とマイコンの確認状況

**基板の型番とマイコンの仕様は未確定。** 引き継ぎ資料にはWT32-SC01 Plus / ESP32-S3、過去の実機記録にはESP32-WROOM-32Eと記載がある。現行の `platformio.ini` は `esp32dev`、ST7796、SPI、GPIO27でのバックライト制御を設定している。これらは現在のプロジェクト設定として扱い、基板の印字と実機情報で型番を照合する。[過去の実機記録](https://github.com/KamikenSamba/pc-smart-dashboard/pull/4)

COMポートの列挙で得られるCH340の情報はUSBシリアル変換器の情報として扱う。マイコンの型番、利用可能なメモリ、LCDバス、タッチの構成は、基板の確認後に記録する。

## 開発順序

1. **通信と運用の安定化**。利用者が確認した機器の自動検出、手動指定、複数ポートがある場合の誤接続防止を実装する。続いて、受信途絶の検出とLCDへの接続状態表示、JSONのキー・型・値域の検証、PC側設定の外部化を進める。
2. **ファームウェアの整理**。シリアル受信・検証とLCD描画を分ける。CPU・RAMの重複描画と座標定数を整理し、受信処理の待ち時間、描画速度、ちらつき、メモリ消費を実機で測る。
3. **車載メーター風UI**。円形ゲージ、針のアニメーション、数値、目盛り、警告領域を試作する。基板を確定したうえでTFT_eSPIのSpriteと部分更新を評価し、起動演出、ダークテーマ、タッチでの画面切り替え、複数テーマを段階的に追加する。
4. **監視項目の拡張**。Windowsと対象ハードウェアで利用できる取得方法を調査し、温度、クロック、GPU・VRAM、ストレージ、ネットワーク、消費電力を順次検討する。
5. **配布と常用**。自動起動、バックグラウンド動作、タスクトレイ、設定GUI、実行ファイル化、GitHub Releases、導入手順とスクリーンショットを整備する。
