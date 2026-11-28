# ESP32の受信データ検証と通信状態の確認

## 自動テスト

リポジトリのルートで `pio run -e esp32dev` を実行し、ファームウェアとArduinoJsonの依存関係を準備します。ホスト用C++17コンパイラで同じ `TelemetryReceiver` をビルドします。以下はWindows x64のPowerShellでZigをPATHから使う例です。

```powershell
zig c++ -target x86_64-windows-gnu -std=c++17 -Wall -Wextra -Wpedantic -Werror -Iinclude `
    -isystem .pio/libdeps/esp32dev/ArduinoJson/src `
    src/telemetry_receiver.cpp test/native/test_telemetry_receiver.cpp `
    -o .pio/telemetry_receiver_tests.exe
if ($LASTEXITCODE -eq 0) { .\.pio\telemetry_receiver_tests.exe }
```

整数・小数・境界値、必須キーや型のエラー、値域外、壊れたJSONと末尾の余分なデータ、分割受信、長すぎる行、5秒の境界、異常フレームの後の復旧、`millis()` の周回を確認します。

## WindowsとESP32での確認

作業ブランチのファームウェアを既存のPlatformIO環境で書き込みます。対象機器のCOM番号は抜き差し時の `python -m serial.tools.list_ports -v` の差分から特定します。例の `COM3` は実機に合わせてください。書き込みとテストの間は、他のシリアルモニターを閉じます。

```powershell
pio run -e esp32dev -t upload --upload-port COM3
```

| 操作 | LCDで確認する結果 |
| --- | --- |
| 送信側を起動せずにボードを起動する | 下部に `WAITING FOR DATA` が出て、約5秒後に `NO SIGNAL` になる。 |
| `python -u pc/monitor.py --port COM3` で監視を開始する | `CONNECTED` になり、既存のCPU・RAMの数値とバーが更新される。 |
| ボードを通電したまま `Ctrl+C` でPC側の監視を止める | 最後の正常データから約5秒後に `NO SIGNAL` になり、数値とバーは直前の値を維持する。 |
| 同じコマンドで監視を再開する | 最初の正常なフレームで `CONNECTED` に戻り、数値とバーが更新される。 |

異常系はPC側の監視を止めてから、次のコマンドを実行します。Python環境には `pc/requirements.txt` の依存関係をインストールしておきます。

```powershell
@'
import time
import serial

with serial.Serial("COM3", 115200, write_timeout=1) as port:
    time.sleep(2)
    cases = [
        (b'{"cpu":25,"ram":46}', "CONNECTED / CPU 25%, RAM 46%", 1.5),
        (b'{"cpu":90}', "DATA ERROR / previous values", 1),
        (b'{"cpu":"90","ram":50}', "DATA ERROR / previous values", 1),
        (b'{"cpu":101,"ram":50}', "DATA ERROR / previous values", 1),
        (b'{"cpu":90,"ram":50', "DATA ERROR, then NO SIGNAL / previous values", 3),
        (b'{"cpu":100,"ram":0}', "CONNECTED / CPU 100%, RAM 0%", 2),
    ]
    for frame, expected, wait in cases:
        print(expected)
        port.write(frame + b"\n")
        time.sleep(wait)
'@ | python -u -
```

`DATA ERROR` は最後の正常受信から5秒未満の間に発生したエラーを示します。エラーが続いてもタイマーは更新されず、5秒で `NO SIGNAL` に切り替わります。受信はLFまたはCRLF区切りです。改行前が256バイトを超える行やNULを含む行は破棄され、次の改行で受信を再開します。

期待と異なる場合は、使用したCOM番号、ファームウェアのコミット、送信した行、LCDに表示された状態・数値を記録します。監視プログラムの送信ログも保存すると、最後の正常フレームからの経過時間を照合できます。
