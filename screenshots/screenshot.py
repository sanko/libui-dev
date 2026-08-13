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
    import mss.tools
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


def _grab_uniform(grab):
    # A screen capture that came back without permission is a solid-color
    # frame; treat it as a failure instead of committing a blank PNG.
    rgb = grab.rgb
    if not rgb:
        return True
    stride = max(1, len(rgb) // 4096)
    first = rgb[0]
    for i in range(0, len(rgb), stride):
        if rgb[i] != first:
            return False
    return True


def _osascript_capture(output):
    # Screen recording on macOS 15+ is attributed to the responsible
    # process, and the runner images pre-authorize only a few of them (see
    # configure-tccdb-macos.sh in actions/runner-images). /usr/bin/osascript
    # is one of those, so run screencapture(1) through it to sidestep the
    # "bypass the system private window picker" dialog that otherwise pops up
    # over the captured window (actions/runner-images#14166).
    tmp = output + ".tmp"
    try:
        subprocess.run(
            ["osascript", "-e", 'do shell script "screencapture -x \\"%s\\""' % tmp],
            check=True, capture_output=True)
        if os.path.isfile(tmp) and os.path.getsize(tmp) > 0:
            os.replace(tmp, output)
            return True
    except Exception:
        pass
    return False


def capture(sct, output):
    if sys.platform == "darwin":
        # Prefer the osascript route above; fall back to mss and only keep a
        # capture that is not a blank frame.
        if _osascript_capture(output):
            return True
        grab = sct.grab(sct.monitors[1])
        if _grab_uniform(grab):
            return False
        mss.tools.to_png(grab.rgb, grab.size, output)
        return True
    sct.shot(output=output)
    return True


def run_and_capture(sct, exe, output, delay):
    proc = subprocess.Popen([exe])
    try:
        time.sleep(delay)
        return capture(sct, output)
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
