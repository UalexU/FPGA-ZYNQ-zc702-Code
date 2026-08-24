"""
Temperature GUI -- Tkinter + embedded matplotlib, fed by sensor.py.

    python gui.py           # read the board
    python gui.py --fake    # sine wave, no hardware needed
"""

import sys
import time
import tkinter as tk
from tkinter import filedialog, messagebox

import pandas as pd
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure

import sensor                      # <-- replaces `import random`

POLL_MS       = 100                # how often we check the queue
WINDOW_POINTS = 30                 # points visible on screen
FAKE          = "--fake" in sys.argv

reader = None                      # the SensorReader, once started
t0     = None                      # monotonic clock at start


# ------------------------------------------------------------- controls

def start_sensors():
    """Start button. Doubles as Stop once running."""
    global reader, t0

    if reader is not None:
        stop_sensors()
        return

    reader = sensor.SensorReader(fake=FAKE).start()
    t0 = time.monotonic()
    StartButton.config(text="Stop")
    status_var.set("Reading..." + ("  (fake data)" if FAKE else ""))


def stop_sensors():
    global reader
    if reader is not None:
        reader.stop()              # ask the background thread to finish
        reader = None
    StartButton.config(text="Start")
    status_var.set("Stopped")


def on_close():
    stop_sensors()                 # don't leave the thread holding the port
    window.destroy()


def toggle_sensor1():
    sensor1_line.set_visible(sensor1_checkbox_enable.get() == 1)
    canvas.draw_idle()


def toggle_sensor2():
    sensor2_line.set_visible(sensor2_checkbox_enable.get() == 1)
    canvas.draw_idle()


def export_data():
    filename = filedialog.asksaveasfilename(
        defaultextension=".xlsx",
        filetypes=[("Excel files", "*.xlsx")]
    )
    if not filename:
        return

    df = pd.DataFrame({
        "Time [Seconds]": x_data,
        "Sensor 1 [C]": y1,
        "Sensor 2 [C]": y2,
    })
    df.to_excel(filename, index=False)
    messagebox.showinfo("Data Saved", f"Data saved to:\n{filename}")


# ----------------------------------------------------------- the poller

def update_plot():
    """Runs every POLL_MS. Drains whatever arrived, redraws, reschedules.

    Never blocks: drain() returns immediately, empty list if nothing came.
    """
    global reader
    changed = False

    if reader is not None:
        if reader.error:
            messagebox.showerror("Sensor error", reader.error)
            stop_sensors()
        else:
            now = time.monotonic() - t0
            for value in reader.drain():
                x_data.append(round(now, 2))
                y1.append(value)
                y2.append(float("nan"))   # keeps all three columns equal length
                changed = True

    if changed:
        sensor1_line.set_data(x_data[-WINDOW_POINTS:], y1[-WINDOW_POINTS:])
        sensor2_line.set_data(x_data[-WINDOW_POINTS:], y2[-WINDOW_POINTS:])
        ax.relim()
        ax.autoscale_view()
        canvas.draw_idle()
        status_var.set(f"{y1[-1]:.1f} C     {len(y1)} samples")

    window.after(POLL_MS, update_plot)    # schedule, then RETURN


# ---------------------------------------------------------------- setup

window = tk.Tk()
window.title("Temp Reading GUI")
window.geometry("1000x800")
window.protocol("WM_DELETE_WINDOW", on_close)

fig = Figure(figsize=(6, 3), dpi=100)
ax = fig.add_subplot(111)

x_data, y1, y2 = [], [], []
sensor1_line, = ax.plot(x_data, y1, marker='o', color='b', label='Sensor 1', visible=True)
sensor2_line, = ax.plot(x_data, y2, marker='o', color='r', label='Sensor 2', visible=False)
ax.set_title("Sensor Temp")
ax.set_xlabel("Time [Seconds]")
ax.set_ylabel("Temp [C]")
ax.legend(loc="upper right")
ax.set_ylim(15, 45)            # initial view; autoscale takes over on real data
ax.grid(True)

canvas = FigureCanvasTkAgg(fig, master=window)
canvas.get_tk_widget().pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

control_frame = tk.Frame(window)
control_frame.pack(side=tk.RIGHT, fill=tk.Y, padx=20)

StartButton = tk.Button(control_frame, text="Start", command=start_sensors, width=18)
StartButton.pack(pady=10)

sensor1_checkbox_enable = tk.IntVar(value=1)
tk.Checkbutton(control_frame, text="Plot Sensor 1 Data",
               variable=sensor1_checkbox_enable,
               command=toggle_sensor1).pack(pady=10)

sensor2_checkbox_enable = tk.IntVar(value=0)
tk.Checkbutton(control_frame, text="Plot Sensor 2 Data",
               variable=sensor2_checkbox_enable,
               command=toggle_sensor2).pack(pady=10)

tk.Button(control_frame, text="Export data to Excel",
          command=export_data, width=18).pack(pady=10)

status_var = tk.StringVar(value="Idle")
tk.Label(control_frame, textvariable=status_var, fg="gray25").pack(pady=20)

canvas.draw()
update_plot()                  # starts the polling loop (no data until Start)
window.mainloop()