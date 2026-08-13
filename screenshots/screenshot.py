#!/usr/bin/env python3
# Capture screenshots of every built libui example program.
#
# Usage:
#     python screenshot.py <build-dir> [-o OUT] [-d DELAY]
#
# The example binaries are looked up by name under <build-dir>, including
# the Release/Debug subdirectories used by multi-config generators such as
# MSVC. Each example is launched, given DELAY seconds to draw its window,
# captured full-screen with mss, and then terminated. The captures land in
# OUT as "<example>.png" files.
#
# Screenshots are best-effort: an example that fails to launch or capture
# produces a warning but does not abort the run, so a broken example cannot
# hide the rest.

import argparse
import os
import subprocess
import sys
import time

try:
    import mss
except ImportError:
    sys.stderr.write("error: this script requires the mss package (pip install mss)\n")
    sys.exit(2)

# Executable names of every add_ui_exe() example target in CMakeLists.txt,
# in build order.
EXAMPLES = [
    "hello-world",
    "window",
    "button",
    "label",
    "checkbox",
    "entry",
    "multiline-entry",
    "box-layout",
    "form-layout",
    "grid-layout",
    "tabs",
    "group",
    "control-destroy",
    "combobox",
    "editable-combobox",
    "radio-buttons",
    "spinbox",
    "slider",
    "progressbar",
    "separator",
    "date-picker",
    "time-picker",
    "color-button",
    "font-button",
    "menu",
    "menu-checkbox",
    "open-file",
    "open-folder",
    "save-file",
    "message-box",
    "error-message-box",
    "timer",
    "queue-main",
    "should-quit",
    "controlgallery",
    "histogram",
    "drawtext",
    "drawbitmap",
    "imagebuffer",
    "datetime",
    "drag-drop",
    "cpp-multithread",
]


def find_example(build_dir, name):
    candidates = [
        os.path.join(build_dir, name),
        os.path.join(build_dir, name + ".exe"),
        os.path.join(build_dir, "Release", name + ".exe"),
        os.path.join(build_dir, "Debug", name + ".exe"),
        os.path.join(build_dir, "RelWithDebInfo", name + ".exe"),
        os.path.join(build_dir, "Release", name),
        os.path.join(build_dir, "Debug", name),
    ]
    for candidate in candidates:
        if os.path.isfile(candidate):
            return candidate
    return None


def capture(sct, output):
    if sys.platform == "darwin":
        # mss can be denied screen-recording access on some systems; fall
        # back to the screencapture(1) utility when that happens.
        try:
            sct.shot(output=output)
        except Exception:
            subprocess.run(["screencapture", "-x", output], check=True)
    else:
        sct.shot(output=output)


def run_and_capture(sct, exe, output, delay):
    proc = subprocess.Popen([exe])
    try:
        time.sleep(delay)
        capture(sct, output)
        return True
    finally:
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait(timeout=3)


def main():
    parser = argparse.ArgumentParser(
        description="Capture screenshots of the built libui examples.")
    parser.add_argument("build_dir",
        help="directory containing the built example binaries")
    parser.add_argument("-o", "--out", default="screenshots",
        help="output directory (default: screenshots)")
    parser.add_argument("-d", "--delay", type=float, default=2.0,
        help="seconds to wait after launching before capturing (default: 2)")
    args = parser.parse_args()

    os.makedirs(args.out, exist_ok=True)

    shots = 0
    with mss.MSS() as sct:
        for name in EXAMPLES:
            exe = find_example(args.build_dir, name)
            if exe is None:
                print("WARN: %s: not found in %s; skipping" % (name, args.build_dir))
                continue
            output = os.path.join(args.out, name + ".png")
            print("capturing %s -> %s" % (name, output))
            try:
                if run_and_capture(sct, exe, output, args.delay):
                    shots += 1
            except Exception as exc:
                print("WARN: %s: capture failed: %s" % (name, exc))

    print("captured %d screenshot(s) into %s" % (shots, args.out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
