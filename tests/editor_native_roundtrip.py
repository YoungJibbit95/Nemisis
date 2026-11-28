"""Drive real SDL editor input on a controlled 1280x800 X11 software-Vulkan display.

This does not stand in for hardware or human visual acceptance.
"""
import argparse
import copy
import ctypes as C
import hashlib
import json
import pathlib
import re
import subprocess
import time


def wait_for(predicate, seconds=30):
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        value = predicate()
        if value:
            return value
        time.sleep(0.2)
    raise RuntimeError("Timed out waiting for " + predicate.__name__)


def window(name):
    result = subprocess.run(["xdotool", "search", "--onlyvisible", "--name", name],
                            text=True, capture_output=True, timeout=5)
    return result.stdout.splitlines()[-1] if result.returncode == 0 and result.stdout.strip() else None


def xdo(*args):
    subprocess.run(["xdotool", *map(str, args)], check=True, timeout=8)


# Xvfb has no window manager. xdotool windowclose destroys the X window
# directly, leaving SDL/Vulkan with a stale native window. Send the same
# WM_DELETE_WINDOW request a real window manager would send instead.
class ClientData(C.Union):
    _fields_ = [("bytes", C.c_char * 20), ("shorts", C.c_short * 10), ("longs", C.c_long * 5)]


class ClientMessage(C.Structure):
    _fields_ = [("type", C.c_int), ("serial", C.c_ulong), ("send_event", C.c_int),
                ("display", C.c_void_p), ("window", C.c_ulong), ("message_type", C.c_ulong),
                ("format", C.c_int), ("data", ClientData)]


class XEvent(C.Union):
    _fields_ = [("client", ClientMessage), ("pad", C.c_long * 24)]


def request_close(wid):
    x = C.CDLL("libX11.so.6")
    x.XOpenDisplay.argtypes = [C.c_char_p]
    x.XOpenDisplay.restype = C.c_void_p
    x.XInternAtom.argtypes = [C.c_void_p, C.c_char_p, C.c_int]
    x.XInternAtom.restype = C.c_ulong
    x.XGetWMProtocols.argtypes = [C.c_void_p, C.c_ulong, C.POINTER(C.POINTER(C.c_ulong)), C.POINTER(C.c_int)]
    x.XGetWMProtocols.restype = C.c_int
    x.XFree.argtypes = [C.c_void_p]
    x.XSendEvent.argtypes = [C.c_void_p, C.c_ulong, C.c_int, C.c_long, C.POINTER(XEvent)]
    x.XSendEvent.restype = C.c_int
    x.XSync.argtypes = [C.c_void_p, C.c_int]
    x.XCloseDisplay.argtypes = [C.c_void_p]
    display = x.XOpenDisplay(None)
    if not display:
        raise RuntimeError("Cannot open X11 test display")
    try:
        target = int(wid)
        protocols = x.XInternAtom(display, b"WM_PROTOCOLS", 1)
        delete = x.XInternAtom(display, b"WM_DELETE_WINDOW", 1)
        supported = C.POINTER(C.c_ulong)()
        count = C.c_int()
        if not protocols or not delete or not x.XGetWMProtocols(display, target, C.byref(supported), C.byref(count)):
            raise RuntimeError("Editor does not advertise WM_DELETE_WINDOW")
        try:
            if delete not in [supported[i] for i in range(count.value)]:
                raise RuntimeError("Editor does not advertise WM_DELETE_WINDOW")
        finally:
            x.XFree(supported)
        event = XEvent()
        event.client.type = 33  # X11 ClientMessage
        event.client.send_event = 1
        event.client.display = display
        event.client.window = target
        event.client.message_type = protocols
        event.client.format = 32
        event.client.data.longs[0] = delete
        event.client.data.longs[1] = 0  # CurrentTime
        if not x.XSendEvent(display, target, 0, 0, C.byref(event)):
            raise RuntimeError("Could not send WM_DELETE_WINDOW")
        x.XSync(display, 0)
    finally:
        x.XCloseDisplay(display)


def click(wid, x, y):
    xdo("mousemove", "--window", wid, x, y)
    xdo("mousedown", 1)
    time.sleep(0.35)
    xdo("mouseup", 1)
    time.sleep(0.35)


def read(path):
    return json.loads(path.read_text(encoding="utf-8"))


def equivalent(actual, expected):
    assert actual.keys() == expected.keys()
    assert actual["schemaVersion"] == expected["schemaVersion"] and actual["id"] == expected["id"]
    assert [o["id"] for o in actual["objects"]] == [o["id"] for o in expected["objects"]]
    for a, b in zip(actual["objects"], expected["objects"]):
        assert a.keys() == b.keys(), (a, b)
        for k in a:
            if k in ("position", "halfExtents"):
                assert len(a[k]) == len(b[k]) and all(abs(x-y) < 1e-5 for x, y in zip(a[k], b[k])), (a, b)
            else:
                assert a[k] == b[k], (a, b)


def run_editor(editor, game, source, output, diagnostics, edit, launch):
    with diagnostics.open("w", encoding="utf-8") as log:
        proc = subprocess.Popen([str(editor), "--map", str(source), "--save-as",
                                 str(output), "--game", str(game)], stdout=log, stderr=subprocess.STDOUT)
        try:
            def find_editor():
                if proc.poll() is not None:
                    raise RuntimeError("Editor exited before its window appeared")
                return window("^NovaCore World Editor - Candidate$")
            wid = wait_for(find_editor)
            xdo("windowfocus", "--sync", wid)
            geometry = subprocess.check_output(["xdotool", "getwindowgeometry", "--shell", wid], text=True)
            assert "WIDTH=1280" in geometry and "HEIGHT=800" in geometry, geometry
            expected_boxes = sum(o["kind"] == "box" for o in read(source)["objects"])
            def renderer_ready():
                if proc.poll() is not None:
                    raise RuntimeError("Editor stopped before rendering")
                for line in diagnostics.read_text(errors="replace").splitlines():
                    match = re.fullmatch(
                        r"NOVACORE_EDITOR_READY backend=vulkan-world frames=(\d+) boxes=(\d+) lines=(\d+) extent=(\d+)x(\d+)",
                        line)
                    if match:
                        frames, boxes, lines, width, height = map(int, match.groups())
                        assert frames >= 3 and boxes == expected_boxes and lines > 0, line
                        assert (width, height) == (1280, 800), line
                        return line
                return None
            print(wait_for(renderer_ready), flush=True)
            if edit:
                click(wid, 90, 143)       # second hierarchy row: door-left
                click(wid, 1250, 185)     # inspector X + 0.25 m
            click(wid, 175, 49)           # Save
            wait_for(output.is_file)
            expected = read(source)
            if edit:
                expected = copy.deepcopy(expected)
                assert expected["objects"][1]["id"] == "door-left"
                expected["objects"][1]["position"][0] += 0.25
            equivalent(read(output), expected)
            print(f"Saved and verified {output.name} ({len(expected['objects'])} stable objects)", flush=True)
            if launch:
                click(wid, 885, 49)       # actual Play Map launcher
                def find_game():
                    return window(r"^NEMISIS \| editor-room$")
                child = wait_for(find_game, 90)
                xdo("windowfocus", "--sync", child)
                time.sleep(1)
                xdo("keydown", "Escape")
                time.sleep(0.4)
                xdo("keyup", "Escape")
                def child_closed():
                    return not find_game()
                wait_for(child_closed, 30)
                def child_reaped():
                    if proc.poll() is not None:
                        raise RuntimeError("Editor exited before reaping the game")
                    for line in diagnostics.read_text(errors="replace").splitlines():
                        match = re.fullmatch(r"NOVACORE_PLAYTEST_EXIT code=(-?\d+)", line)
                        if match:
                            assert int(match.group(1)) == 0, line
                            return line
                    return None
                print(wait_for(child_reaped), flush=True)
                assert "Authored world loaded: editor-room spawn=spawn" in diagnostics.read_text(errors="replace")
                xdo("windowfocus", "--sync", wid)
            request_close(wid)
            assert proc.wait(timeout=20) == 0, "Editor did not shut down cleanly"
            assert "Renderer shutdown" in diagnostics.read_text(errors="replace"), "Renderer cleanup missing"
        except Exception:
            print(f"Editor diagnostic before teardown ({diagnostics}):", flush=True)
            print(diagnostics.read_text(errors="replace")[-12000:], flush=True)
            raise
        finally:
            if proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait(timeout=5)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--editor", type=pathlib.Path, required=True)
    parser.add_argument("--game", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()
    editor, game, out = args.editor.resolve(), args.game.resolve(), args.output.resolve()
    game_digest = hashlib.sha256(game.read_bytes()).hexdigest()
    out.mkdir(parents=True, exist_ok=True)
    source = pathlib.Path("configs/maps/editor-room.world.json").resolve()
    modified = out / "modified.world.json"
    reopened = out / "reopened.world.json"
    for path in (modified, reopened):
        assert not path.exists(), f"Refusing to overwrite an existing test artifact: {path}"
    run_editor(editor, game, source, modified, out / "editor-save.log", edit=True, launch=True)
    run_editor(editor, game, modified, reopened, out / "editor-reopen.log", edit=False, launch=False)
    equivalent(read(modified), read(reopened))
    for label, path in (("original", source), ("modified", reopened)):
        subprocess.run([str(game), "--map", str(path), "--check-map"], check=True, timeout=30)
        with (out / (label + "-game-vulkan.log")).open("w", encoding="utf-8") as log:
            subprocess.run([str(game), "--map", str(path), "--vulkan-smoke"],
                           check=True, stdout=log, stderr=subprocess.STDOUT, timeout=120)
        log_text = (out / (label + "-game-vulkan.log")).read_text(errors="replace")
        assert "Authored Vulkan smoke passed: world=editor-room spawn=spawn" in log_text
    for log in out.glob("*.log"):
        assert not re.search(r"Validation Error|ERROR / SPEC|VUID-|X Error of failed request", log.read_text(errors="replace"), re.I), log
    assert hashlib.sha256(game.read_bytes()).hexdigest() == game_digest, "Game executable changed during round-trip"
    print("Editor input/save/reopen/Play Map + unchanged game executable original/modified Vulkan smoke passed")


if __name__ == "__main__":
    main()
