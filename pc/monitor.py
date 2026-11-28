import argparse
import json
import re
import time
from functools import partial

import psutil
import serial
from serial.tools import list_ports

BAUD_RATE = 115200
RECONNECT_INTERVAL = 2
STARTUP_DELAY = 2
WRITE_TIMEOUT = 1


def find_usb_port(vid, pid, identity_field, identity):
    matches = [
        port.device
        for port in list_ports.comports()
        if (port.vid, port.pid) == (vid, pid)
        and getattr(port, identity_field) == identity
    ]
    if not matches:
        raise serial.SerialException("指定したUSB機器のポートが見つかりません")
    if len(matches) > 1:
        raise serial.SerialException(
            f"指定したUSB識別情報に複数のポートが一致しました: {', '.join(matches)}"
        )
    return matches[0]


def run_monitor(resolve_port):
    try:
        while True:
            try:
                # Windows can assign a different COM number when USB reconnects.
                port = resolve_port()
                with serial.Serial(
                    port, BAUD_RATE, write_timeout=WRITE_TIMEOUT
                ) as ser:
                    print(f"シリアル接続: {port}")
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


def main(argv=None):
    parser = argparse.ArgumentParser(description="ESP32へPCのCPU・RAM使用率を送信します。")
    selection = parser.add_mutually_exclusive_group(required=True)
    selection.add_argument("--port", metavar="COM", help="ポートを手動指定します。")
    selection.add_argument(
        "--usb-serial",
        nargs=2,
        metavar=("VID:PID", "SERIAL"),
        help="VID/PIDとUSBシリアル番号でポートを検出します。",
    )
    selection.add_argument(
        "--usb-location",
        nargs=2,
        metavar=("VID:PID", "LOCATION"),
        help="VID/PIDとUSB接続位置でポートを検出します。",
    )
    args = parser.parse_args(argv)

    if args.port is not None:
        if not args.port.strip():
            parser.error("--port に空の値は指定できません。")
        run_monitor(lambda: args.port)
        return

    if args.usb_serial:
        usb_id, identity = args.usb_serial
        identity_field = "serial_number"
    else:
        usb_id, identity = args.usb_location
        identity_field = "location"

    if not re.fullmatch(r"[0-9a-fA-F]{4}:[0-9a-fA-F]{4}", usb_id):
        parser.error("VID:PID は4桁の16進数2組で指定してください。")
    if not identity.strip():
        parser.error("USBシリアル番号またはUSB接続位置を指定してください。")
    vid, pid = (int(part, 16) for part in usb_id.split(":"))
    run_monitor(partial(find_usb_port, vid, pid, identity_field, identity))


if __name__ == "__main__":
    main()
