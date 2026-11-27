import json
import time

import psutil
import serial

PORT = "COM3"
BAUD_RATE = 115200
RECONNECT_INTERVAL = 2
STARTUP_DELAY = 2
WRITE_TIMEOUT = 1


def main():
    try:
        while True:
            try:
                with serial.Serial(
                    PORT, BAUD_RATE, write_timeout=WRITE_TIMEOUT
                ) as ser:
                    print(f"シリアル接続: {PORT}")
                    # Opening the port can reset the ESP32; wait before sending.
                    time.sleep(STARTUP_DELAY)

                    while True:
                        cpu_usage = psutil.cpu_percent(interval=1)
                        ram_usage = psutil.virtual_memory().percent

                        data = {"cpu": cpu_usage, "ram": ram_usage}
                        message = (json.dumps(data) + "\n").encode("utf-8")
                        if ser.write(message) != len(message):
                            raise serial.SerialException("送信データが不完全です")
                        # write() sends the data; flush() can block indefinitely
                        # on some drivers even when write_timeout is configured.
                        print(message.decode("utf-8").strip())

            except (serial.SerialException, OSError) as error:
                print(f"シリアル通信エラー: {error}")
                print(f"{RECONNECT_INTERVAL}秒後に再接続します")
                time.sleep(RECONNECT_INTERVAL)

    except KeyboardInterrupt:
        print("モニタリングを終了しました")


if __name__ == "__main__":
    main()
