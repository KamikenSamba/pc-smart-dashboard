import psutil
import serial
import json
import time

try:
    with serial.Serial("COM3", 115200) as ser:
        time.sleep(2)

        try:
            while True:
                cpu_usage = psutil.cpu_percent(interval=1)
                ram_usage = psutil.virtual_memory().percent

                data = {"cpu": cpu_usage, "ram": ram_usage}
                message = json.dumps(data) + "\n"

                ser.write(message.encode())
                ser.flush()
                print(message.strip())

        except KeyboardInterrupt:
            print("モニタリングを終了しました")

except serial.SerialException as e:
    print(f"シリアル通信エラー: {e}")