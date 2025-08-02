import serial
import matplotlib.pyplot as plt
from matplotlib.patches import Circle
import numpy as np

# --- CONFIGURATION ---
SERIAL_PORT = 'COM8'        # Change this to your ESP32 port (Check Arduino IDE)
BAUD_RATE = 115200          # Must match ESP32

# Known positions of the 3 Access Points (in meters)
AP1 = (0, 0)      # Example: Bottom-left corner
AP2 = (4, 0)      # Bottom-right
AP3 = (2, 3)      # Top-center

# Setup Serial Connection
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Connected to {SERIAL_PORT}")
except:
    print("❌ Could not open serial port. Check port name!")
    exit()

# Setup Plot
fig, ax = plt.subplots(figsize=(8, 6))
ax.set_xlim(0, 5)
ax.set_ylim(0, 4)
ax.set_xlabel("X (meters)")
ax.set_ylabel("Y (meters)")
ax.set_title("Real-Time ESP32 Position Tracking")
ax.grid(True)

# Plot APs
ap_x = [AP1[0], AP2[0], AP3[0]]
ap_y = [AP1[1], AP2[1], AP3[1]]
ax.scatter(ap_x, ap_y, c='blue', s=100, label="Access Points", zorder=5)

for i, ap in enumerate([AP1, AP2, AP3]):
    ax.text(ap[0], ap[1] + 0.1, f"AP{i+1}", ha='center')

tracked_point, = ax.plot([], [], 'ro', markersize=10, label="ESP32 Device")
plt.legend()

def trilaterate(ap1, d1, ap2, d2, ap3, d3):
    """
    Simple trilateration using least squares (2D)
    Returns estimated (x, y) position
    """
    A = 2 * (ap2[0] - ap1[0])
    B = 2 * (ap2[1] - ap1[1])
    C = d1**2 - d2**2 - ap1[0]**2 + ap2[0]**2 - ap1[1]**2 + ap2[1]**2
    D = 2 * (ap3[0] - ap2[0])
    E = 2 * (ap3[1] - ap2[1])
    F = d2**2 - d3**2 - ap2[0]**2 + ap3[0]**2 - ap2[1]**2 + ap3[1]**2

    if A*E - B*D == 0:
        return None  # No solution

    x = (C*E - F*B) / (A*E - B*D)
    y = (A*F - D*C) / (A*E - B*D)
    return (x, y)

# Real-time update loop
print("Waiting for data...")

while True:
    line = ser.readline().decode('utf-8', errors='ignore').strip()
    if "DIST:" in line:
        try:
            # Extract distances
            data = line.replace("DIST:", "").split(",")
            d1, d2, d3 = map(float, data)

            print(f"Received: d1={d1:.2f}, d2={d2:.2f}, d3={d3:.2f}")

            # Trilateration
            pos = trilaterate(AP1, d1, AP2, d2, AP3, d3)
            if pos:
                x, y = pos
                # Safety: keep within bounds
                if 0 <= x <= 5 and 0 <= y <= 4:
                    tracked_point.set_data([x], [y])
                    ax.cla()  # Clear and redraw
                    ax.set_xlim(0, 5)
                    ax.set_ylim(0, 4)
                    ax.grid(True)
                    ax.set_xlabel("X (meters)")
                    ax.set_ylabel("Y (meters)")
                    ax.set_title("Real-Time ESP32 Position Tracking")

                    # Replot APs
                    ax.scatter(ap_x, ap_y, c='blue', s=100)
                    for i, ap in enumerate([AP1, AP2, AP3]):
                        ax.text(ap[0], ap[1] + 0.1, f"AP{i+1}", ha='center')

                    # Plot circles (optional)
                    for ap, d in zip([AP1, AP2, AP3], [d1, d2, d3]):
                        circle = Circle(ap, d, color='gray', fill=False, linestyle='--', alpha=0.5)
                        ax.add_patch(circle)

                    # Plot current position
                    ax.plot(x, y, 'ro', markersize=10, label="ESP32")
                    plt.legend()
                    plt.pause(0.01)  # Update plot

        except Exception as e:
            print("Error processing data:", e)

plt.close()