import serial
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np
import time

# --- Configuration ---
SERIAL_PORT = 'COM4'
BAUD_RATE = 115200

# AP coordinates
AP_COORDS = {
    "narzo": np.array([8.0, 0.0]),
    "hello": np.array([0.0, 5.0]),
    "A": np.array([0.0, 0.0])
}
AP_NAMES = ["narzo", "hello", "A"]
P1, P2, P3 = AP_COORDS[AP_NAMES[0]], AP_COORDS[AP_NAMES[1]], AP_COORDS[AP_NAMES[2]]

# --- Global variables ---
ser = None
esp32_position = np.array([0.0, 0.0])
distances = [0.0, 0.0, 0.0]
_history = []
_partial_line = b''

# --- Serial Connection ---
def connect_serial_once():
    global ser
    try:
        s = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.5)
        time.sleep(1.5)  # Give ESP32 time to reset
        s.reset_input_buffer()
        print(f"Connected to {SERIAL_PORT}")
        return s
    except:
        return None

# --- Trilateration Solver ---
def solve_trilateration(p1, p2, p3, r1, r2, r3):
    if r1 <= 0 or r2 <= 0 or r3 <= 0:
        return None
    A = 2*(p2[0]-p1[0])
    B = 2*(p2[1]-p1[1])
    C = r1**2 - r2**2 + p2[0]**2 - p1[0]**2 + p2[1]**2 - p1[1]**2
    D = 2*(p3[0]-p2[0])
    E = 2*(p3[1]-p2[1])
    F = r2**2 - r3**2 + p3[0]**2 - p2[0]**2 + p3[1]**2 - p2[1]**2
    denom = A*E - B*D
    if abs(denom) < 1e-9:
        return None
    x = (C*E - F*B)/denom
    y = (A*F - D*C)/denom
    return np.array([x, y])

# --- Parse Serial Data ---
def parse_data_line(line):
    if not line.startswith("DATA:"):
        return None
    parts = line[len("DATA:"):].strip().split(',')
    if len(parts) != 3:
        return None
    try:
        return [float(p) for p in parts]
    except:
        return None

# --- Update Data from ESP32 ---
def update_data():
    global ser, distances, esp32_position, _partial_line, _history
    if ser is None or not ser.is_open:
        ser = connect_serial_once()
        return
    try:
        while ser.in_waiting > 0:
            raw = ser.read(ser.in_waiting)
            if not raw:
                break
            _partial_line += raw
            while b'\n' in _partial_line:
                line_bytes, _partial_line = _partial_line.split(b'\n', 1)
                try:
                    line = line_bytes.decode('utf-8', errors='replace').strip()
                except:
                    continue
                parsed = parse_data_line(line)
                if parsed is not None:
                    new_dists = [0.0 if v <= 0.001 else v for v in parsed]
                    distances[:] = new_dists
                    pos = solve_trilateration(P1, P2, P3, distances[0], distances[1], distances[2])
                    if pos is not None:
                        esp32_position[:] = pos
                        _history.append(pos.copy())
                        if len(_history) > 100:
                            _history.pop(0)
    except serial.SerialException:
        try:
            ser.close()
        except:
            pass
        ser = None

# --- Matplotlib Plot Setup ---
fig, ax = plt.subplots()
ax.set_aspect('equal', adjustable='box')

# Plot Access Points
ap_scatter = ax.scatter([P1[0], P2[0], P3[0]], [P1[1], P2[1], P3[1]], s=100, c='blue', label='Access Points')
for i, name in enumerate(AP_NAMES):
    ax.text(AP_COORDS[name][0] + 0.3, AP_COORDS[name][1] + 0.3, name, color='blue')

# ESP32 Dot
esp32_dot, = ax.plot([], [], 'ro', markersize=8, label='ESP32')

# Distance circles
circle1 = plt.Circle(P1, 0, fill=False, linestyle='--', alpha=0.6)
circle2 = plt.Circle(P2, 0, fill=False, linestyle='--', alpha=0.6)
circle3 = plt.Circle(P3, 0, fill=False, linestyle='--', alpha=0.6)
ax.add_artist(circle1)
ax.add_artist(circle2)
ax.add_artist(circle3)

# Distance text labels near APs
dist_text1 = ax.text(0, 0, '', fontsize=9, ha='center', va='center')
dist_text2 = ax.text(0, 0, '', fontsize=9, ha='center', va='center')
dist_text3 = ax.text(0, 0, '', fontsize=9, ha='center', va='center')

# Historical trail
trail, = ax.plot([], [], 'r-', alpha=0.5, lw=1)

# Uncertainty ellipse
uncertainty_ellipse = plt.Circle((0,0),0,fill=False,color='orange',linestyle=':',alpha=0.5)
ax.add_artist(uncertainty_ellipse)

# Live data text in GUI
live_text = ax.text(0.02, 0.98, '', transform=ax.transAxes,
                    fontsize=9, verticalalignment='top',
                    bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

# --- Init function ---
def init():
    ax.set_xlabel("X (m)"); ax.set_ylabel("Y (m)")
    ax.set_title("ESP32 WiFi Trilateration")
    ax.grid(True); ax.legend()
    all_pts = np.array([P1,P2,P3])
    minc = np.min(all_pts,axis=0)-5
    maxc = np.max(all_pts,axis=0)+5
    ax.set_xlim(minc[0], maxc[0]); ax.set_ylim(minc[1], maxc[1])
    esp32_dot.set_data([], [])
    trail.set_data([], [])
    live_text.set_text("")
    return esp32_dot, circle1, circle2, circle3, dist_text1, dist_text2, dist_text3, trail, uncertainty_ellipse, live_text

# --- Animation function ---
def animate(i):
    update_data()
    esp32_dot.set_data([esp32_position[0]], [esp32_position[1]])

    # Update distance circles
    circle1.set_radius(distances[0]); circle2.set_radius(distances[1]); circle3.set_radius(distances[2])

    # Distance labels
    for d, pos, text in zip(distances, [P1,P2,P3], [dist_text1, dist_text2, dist_text3]):
        if d > 0.001:
            midpoint = pos + (esp32_position - pos)*0.5
            text.set_position(midpoint)
            text.set_text(f'{d:.1f} m')
        else:
            text.set_text('')

    # Historical trail
    if len(_history) > 1:
        hist_array = np.array(_history)
        trail.set_data(hist_array[:,0], hist_array[:,1])

    # Uncertainty ellipse
    if len(distances) == 3 and all(d > 0.001 for d in distances):
        est = esp32_position
        rad = np.mean([abs(np.linalg.norm(est-P1)-distances[0]),
                       abs(np.linalg.norm(est-P2)-distances[1]),
                       abs(np.linalg.norm(est-P3)-distances[2])])
        uncertainty_ellipse.center = (est[0], est[1])
        uncertainty_ellipse.set_radius(rad)

    # Live data text in GUI
    text_lines = [f"ESP32 Position: ({esp32_position[0]:.2f}, {esp32_position[1]:.2f})"]
    for name, dist in zip(AP_NAMES, distances):
        text_lines.append(f"{name}: {dist:.2f} m")
    live_text.set_text("\n".join(text_lines))

    return esp32_dot, circle1, circle2, circle3, dist_text1, dist_text2, dist_text3, trail, uncertainty_ellipse, live_text

# --- Main ---
if __name__ == "__main__":
    ser = connect_serial_once()
    ani = animation.FuncAnimation(fig, animate, init_func=init, interval=200, blit=False)
    plt.show()
    try: ser.close()
    except: pass
