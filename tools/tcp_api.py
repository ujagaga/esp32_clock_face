#!/usr/bin/env python3
"""Command-line client for the ESP32 clock JSON-over-TCP API (see tcp_server.cpp).

Examples:
  tcp_api.py 192.168.4.1 list
  tcp_api.py 192.168.4.1 setdisplay eyes1.bin
  tcp_api.py 192.168.4.1 setdisplay clock
  tcp_api.py 192.168.4.1 setled 00ff00
  tcp_api.py 192.168.4.1 setbl 40
  tcp_api.py 192.168.4.1 httpserver off
"""

import argparse
import json
import socket
import sys


def send(host, port, obj, timeout):
    """Send one JSON command line, return the device's parsed JSON reply."""
    line = (json.dumps(obj) + "\n").encode()
    with socket.create_connection((host, port), timeout=timeout) as s:
        s.settimeout(timeout)
        s.sendall(line)
        buf = b""
        while b"\n" not in buf:
            chunk = s.recv(256)
            if not chunk:
                break
            buf += chunk
    reply = buf.split(b"\n", 1)[0].decode(errors="replace").strip()
    try:
        return json.loads(reply)
    except json.JSONDecodeError:
        return {"_raw": reply}


def build_command(args):
    """Map the parsed subcommand to the JSON object the device expects."""
    if args.cmd == "setdisplay":
        return {"cmd": "setdisplay", "img": args.img}
    if args.cmd == "setled":
        return {"cmd": "setled", "c": args.color}
    if args.cmd == "setbl":
        return {"cmd": "setbl", "v": args.value}
    if args.cmd == "httpserver":
        if args.state == "status":
            return {"cmd": "httpserver"}
        return {"cmd": "httpserver", "enable": args.state == "on"}
    # list / getdisplay / gettime / flashbl / flipscreen take no arguments
    return {"cmd": args.cmd}


def main():
    p = argparse.ArgumentParser(
        description="Send a JSON command to the ESP32 clock TCP API.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    p.add_argument("host", help="Device IP address or hostname")
    p.add_argument("-p", "--port", type=int, default=333, help="TCP API port")
    p.add_argument("-t", "--timeout", type=float, default=5.0, help="Socket timeout (s)")

    sub = p.add_subparsers(dest="cmd", required=True, metavar="command")
    sub.add_parser("list", help="List *.bin images on the SD card")
    sub.add_parser("getdisplay", help="Show the active display name")
    sub.add_parser("gettime", help="Show the device time")
    sub.add_parser("flashbl", help="Flash the backlight")
    sub.add_parser("flipscreen", help="Rotate the screen 180 degrees")

    sd = sub.add_parser("setdisplay", help="Show an image, or 'clock'")
    sd.add_argument("img", nargs="?", default="clock", help="Image filename or 'clock'")

    sl = sub.add_parser("setled", help="Set the RGB LED color")
    sl.add_argument("color", help="6 hex digits, e.g. 00ff00")

    sb = sub.add_parser("setbl", help="Set backlight brightness 0..100")
    sb.add_argument("value", type=int, help="Brightness 0..100")

    hs = sub.add_parser("httpserver", help="Start/stop/query the HTTP server")
    hs.add_argument("state", choices=["on", "off", "status"], help="Action")

    args = p.parse_args()

    try:
        reply = send(args.host, args.port, build_command(args), args.timeout)
    except OSError as e:
        print(f"Connection error: {e}", file=sys.stderr)
        return 2

    print(json.dumps(reply, indent=2))
    # Non-zero exit on an error reply, handy for scripting.
    if "error" in reply or reply.get("ok") is False:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
