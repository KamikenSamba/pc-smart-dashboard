import serial
import time
import json

with serial.Serial("COM3", 115200) as ser:
    time.sleep(2)
    data = {"cpu": 25, "ram": 46}
    message = json.dumps(data) + "\n"
    ser.write(message.encode())
    ser.flush()
    time.sleep(1)