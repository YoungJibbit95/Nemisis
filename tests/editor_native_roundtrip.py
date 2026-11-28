"""Drive real SDL editor input on a controlled 1280x800 X11 software-Vulkan display.

This does not stand in for hardware or human visual acceptance.
"""
import argparse
import copy
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
            xdo("windowclose", wid)
            assert proc.wait(timeout=20) == 0
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
        assert not re.search(r"Validation Error|ERROR / SPEC|VUID-", log.read_text(errors="replace"), re.I), log
    assert hashlib.sha256(game.read_bytes()).hexdigest() == game_digest, "Game executable changed during round-trip"
    print("Editor input/save/reopen/Play Map + unchanged game executable original/modified Vulkan smoke passed")


if __name__ == "__main__":
    main()
