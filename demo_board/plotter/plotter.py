#!/usr/bin/env python3
"""
ZeoDevBoardsensor plotter.
Listens for UDP broadcast JSON packets from the ESP32-S3 and plots
in the terminal using plotext.
"""

import json
import socket
import threading
import time
from collections import deque

import plotext as plt

UDP_PORT = 12345
REFRESH_HZ = 4  # terminal refresh rate

# Rolling window sizes
IMU_WINDOW = 500    # 5 seconds at 100 Hz
ENV_WINDOW = 60     # 60 seconds at 1 Hz

# IMU deques
ax_data = deque(maxlen=IMU_WINDOW)
ay_data = deque(maxlen=IMU_WINDOW)
az_data = deque(maxlen=IMU_WINDOW)
gx_data = deque(maxlen=IMU_WINDOW)
gy_data = deque(maxlen=IMU_WINDOW)
gz_data = deque(maxlen=IMU_WINDOW)
imu_ts = deque(maxlen=IMU_WINDOW)

# ENV deques
temp_data = deque(maxlen=ENV_WINDOW)
hum_data = deque(maxlen=ENV_WINDOW)
press_data = deque(maxlen=ENV_WINDOW)
env_ts = deque(maxlen=ENV_WINDOW)

t0 = None
lock = threading.Lock()
pkt_count = {"imu": 0, "env": 0}


def udp_receiver():
    """Background thread: receive UDP packets and parse JSON into deques."""
    global t0
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("0.0.0.0", UDP_PORT))

    while True:
        data, addr = sock.recvfrom(1024)
        try:
            pkt = json.loads(data.decode())
        except (json.JSONDecodeError, UnicodeDecodeError):
            continue

        with lock:
            ts = pkt.get("ts", 0)
            if t0 is None:
                t0 = ts
            t_sec = (ts - t0) / 1000.0

            if pkt.get("type") == "imu":
                imu_ts.append(t_sec)
                ax_data.append(pkt.get("ax", 0))
                ay_data.append(pkt.get("ay", 0))
                az_data.append(pkt.get("az", 0))
                gx_data.append(pkt.get("gx", 0))
                gy_data.append(pkt.get("gy", 0))
                gz_data.append(pkt.get("gz", 0))
                pkt_count["imu"] += 1
            elif pkt.get("type") == "env":
                env_ts.append(t_sec)
                temp_data.append(pkt.get("temp", 0))
                hum_data.append(pkt.get("hum", 0))
                press_data.append(pkt.get("press", 0))
                pkt_count["env"] += 1


def draw():
    """Redraw all plots in the terminal."""
    with lock:
        imu_t = list(imu_ts)
        ax, ay, az = list(ax_data), list(ay_data), list(az_data)
        gx, gy, gz = list(gx_data), list(gy_data), list(gz_data)
        env_t = list(env_ts)
        temp = list(temp_data)
        hum = list(hum_data)
        press = list(press_data)
        n_imu = pkt_count["imu"]
        n_env = pkt_count["env"]

    plt.clf()
    plt.subplots(3, 2)
    plt.theme("dark")

    # ── Row 1, Col 1: Accelerometer ──
    plt.subplot(1, 1)
    plt.title("Accelerometer (m/s²)")
    if imu_t:
        plt.plot(imu_t, ax, label="X", color="red")
        plt.plot(imu_t, ay, label="Y", color="green")
        plt.plot(imu_t, az, label="Z", color="blue")
    else:
        plt.plot([0], [0])
    plt.xlabel("Time (s)")

    # ── Row 1, Col 2: Gyroscope ──
    plt.subplot(1, 2)
    plt.title("Gyroscope (deg/s)")
    if imu_t:
        plt.plot(imu_t, gx, label="X", color="red")
        plt.plot(imu_t, gy, label="Y", color="green")
        plt.plot(imu_t, gz, label="Z", color="blue")
    else:
        plt.plot([0], [0])
    plt.xlabel("Time (s)")

    # ── Row 2, Col 1: Temperature ──
    plt.subplot(2, 1)
    plt.title("Temperature (°C)")
    if env_t:
        plt.plot(env_t, temp, color="red")
    else:
        plt.plot([0], [0])
    plt.xlabel("Time (s)")

    # ── Row 2, Col 2: Humidity ──
    plt.subplot(2, 2)
    plt.title("Humidity (%RH)")
    if env_t:
        plt.plot(env_t, hum, color="cyan")
    else:
        plt.plot([0], [0])
    plt.xlabel("Time (s)")

    # ── Row 3, Col 1: Pressure ──
    plt.subplot(3, 1)
    plt.title("Pressure (hPa)")
    if env_t:
        plt.plot(env_t, press, color="green")
    else:
        plt.plot([0], [0])
    plt.xlabel("Time (s)")

    # ── Row 3, Col 2: Status ──
    plt.subplot(3, 2)
    plt.title("Status")
    plt.plot([0], [0])  # placeholder
    plt.xlabel(f"IMU pkts: {n_imu}  |  ENV pkts: {n_env}")

    plt.show()


def main():
    print(f"ZeoDevBoard Plotter — listening on UDP port {UDP_PORT}")
    print("Press Ctrl+C to quit.\n")

    receiver = threading.Thread(target=udp_receiver, daemon=True)
    receiver.start()

    try:
        while True:
            draw()
            time.sleep(1.0 / REFRESH_HZ)
    except KeyboardInterrupt:
        print("\nDone.")


if __name__ == "__main__":
    main()
