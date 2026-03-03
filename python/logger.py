import argparse
import csv
import time
from collections import deque

import serial
import serial.tools.list_ports


def list_ports():
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("No serial ports found.")
        return
    for p in ports:
        print(f"- {p.device} | {p.description}")


def pick_port(hint: str | None) -> str:
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        raise SystemExit("No serial ports found. Is the INDbox connected?")

    if hint:
        h = hint.lower()
        for p in ports:
            if h in (p.device or "").lower() or h in (p.description or "").lower():
                return p.device

    # Heuristic: prefer real USB-UART and prefer 'cu.' on macOS
    def score(p):
        dev = (p.device or "").lower()
        desc = (p.description or "").lower()
        s = 0
        if "usbserial" in dev or "usbmodem" in dev:
            s += 50
        if (
            "cp210" in desc
            or "silabs" in desc
            or "ch340" in desc
            or "usb to uart" in desc
        ):
            s += 40
        if dev.startswith("cu."):
            s += 10
        if dev.startswith("tty."):
            s += 2
        if "debug-console" in dev or "debug-console" in desc:
            s -= 100
        return s

    ports.sort(key=score, reverse=True)
    return ports[0].device


def parse_line(line: str):
    # expected: b1,b2,pot,dist
    parts = [p.strip() for p in line.strip().split(",")]
    if len(parts) < 4:
        return None
    try:
        b1 = 1 if int(parts[0]) != 0 else 0
        b2 = 1 if int(parts[1]) != 0 else 0
        pot = int(parts[2])
        dist = float(parts[3])
        return b1, b2, pot, dist
    except Exception:
        return None


def open_serial(port: str, baud: int):
    ser = serial.Serial(port=port, baudrate=baud, timeout=0.2)
    # flush junk
    ser.reset_input_buffer()
    return ser


def run_record_csv(ser, out_path: str, duration_s: float | None):
    t0 = time.time()
    ok = bad = 0

    with open(out_path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["t_sec", "b1", "b2", "pot", "dist", "raw"])

        print(f"Recording to {out_path} ...")
        print(
            "Stop with Ctrl+C" if duration_s is None else f"Duration: {duration_s:.1f}s"
        )

        try:
            while True:
                if duration_s is not None and (time.time() - t0) >= duration_s:
                    break

                raw = ser.readline().decode("utf-8", errors="ignore").strip()
                if not raw:
                    continue
                if raw.startswith("b1") or raw.startswith("#"):
                    continue

                parsed = parse_line(raw)
                if parsed is None:
                    bad += 1
                    continue

                ok += 1
                b1, b2, pot, dist = parsed
                t = time.time() - t0
                writer.writerow([f"{t:.4f}", b1, b2, pot, f"{dist:.4f}", raw])

        except KeyboardInterrupt:
            pass

    print(f"Done. ok={ok}, bad={bad}")


def run_live_plot(ser, window_s: float, sample_hz_hint: float):
    import matplotlib.pyplot as plt

    # rolling buffers
    t_buf = deque()
    pot_buf = deque()
    dist_buf = deque()
    b1_buf = deque()
    b2_buf = deque()

    t0 = time.time()
    ok = bad = 0

    plt.ion()
    _, ax = plt.subplots()
    (pot_line,) = ax.plot([], [], label="pot (raw)")
    (dist_line,) = ax.plot([], [], label="dist")
    ax.set_xlabel("time (s)")
    ax.set_ylabel("value")
    ax.legend(loc="upper right")

    # update interval
    redraw_every = max(1, int(sample_hz_hint / 10))  # ~10 fps redraw
    n = 0

    print("Live plot running. Stop with Ctrl+C")
    try:
        while True:
            raw = ser.readline().decode("utf-8", errors="ignore").strip()
            if not raw:
                continue
            if raw.startswith("b1") or raw.startswith("#"):
                continue

            parsed = parse_line(raw)
            if parsed is None:
                bad += 1
                continue

            ok += 1
            b1, b2, pot, dist = parsed
            t = time.time() - t0

            t_buf.append(t)
            pot_buf.append(pot)
            dist_buf.append(dist)
            b1_buf.append(b1)
            b2_buf.append(b2)

            # drop old samples outside window
            while t_buf and (t - t_buf[0]) > window_s:
                t_buf.popleft()
                pot_buf.popleft()
                dist_buf.popleft()
                b1_buf.popleft()
                b2_buf.popleft()

            n += 1
            if n % redraw_every == 0:
                pot_line.set_data(list(t_buf), list(pot_buf))
                dist_line.set_data(list(t_buf), list(dist_buf))

                ax.relim()
                ax.autoscale_view()
                ax.set_title(f"INDbox live (ok={ok}, bad={bad})  |  last='{raw[:40]}'")
                plt.pause(0.001)

    except KeyboardInterrupt:
        print(f"\nStopped. ok={ok}, bad={bad}")


def main():
    ap = argparse.ArgumentParser(
        description="INDbox stability logger: live plot or record CSV"
    )
    ap.add_argument("--list", action="store_true", help="List serial ports and exit")
    ap.add_argument(
        "--port",
        default=None,
        help="Serial port (e.g. /dev/cu.usbserial-0001). If omitted, auto-pick.",
    )
    ap.add_argument(
        "--hint",
        default=None,
        help="Substring to match port device/description (e.g. usbserial, CP2102)",
    )
    ap.add_argument(
        "--baud", type=int, default=115200, help="Baud rate (default 115200)"
    )
    ap.add_argument(
        "--mode",
        choices=["live", "csv"],
        default="live",
        help="Mode: live plot or CSV record",
    )
    ap.add_argument(
        "--out", default="indbox_log.csv", help="CSV output path (mode=csv)"
    )
    ap.add_argument(
        "--duration",
        type=float,
        default=None,
        help="Record duration in seconds (mode=csv). Default: until Ctrl+C",
    )
    ap.add_argument(
        "--window",
        type=float,
        default=10.0,
        help="Live plot window in seconds (mode=live)",
    )
    ap.add_argument(
        "--hz",
        type=float,
        default=30.0,
        help="Expected sample rate (for plotting refresh only)",
    )
    args = ap.parse_args()

    if args.list:
        list_ports()
        return

    port = args.port or pick_port(args.hint)
    print(f"Using port: {port} @ {args.baud}")
    ser = open_serial(port, args.baud)

    try:
        if args.mode == "csv":
            run_record_csv(ser, args.out, args.duration)
        else:
            run_live_plot(ser, args.window, args.hz)
    finally:
        try:
            ser.close()
        except Exception:
            pass


if __name__ == "__main__":
    main()
