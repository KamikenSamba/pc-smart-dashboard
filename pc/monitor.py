import psutil
import json

try:
    while True:
        cpu_usage = psutil.cpu_percent(interval=1)
        ram_usage = psutil.virtual_memory().percent

        data = {"cpu": cpu_usage, "ram": ram_usage}
        json_data = json.dumps(data)
        print(json_data)

except KeyboardInterrupt:
    print("Monitoring stopped.")