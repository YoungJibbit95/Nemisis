"""Run the hosted rotation with 12 direct clients, then a duel over lossy UDP."""

import contextlib
import heapq
import queue
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time


MATCH_PROTOCOL = 0x4D415405
READY_HEADER = struct.pack("<IB", MATCH_PROTOCOL, 6)
SNAPSHOT_HEADER = struct.Struct("<IBQBBBII")
DUEL_SECONDS = 20


def stop_process(process):
    if process.poll() is None:
        process.kill()
    process.wait(timeout=5)


def server_lines(stream, output):
    for line in stream:
        print(line, end="", flush=True)
        output.put(line.strip())
    output.put(None)


def wait_for_line(output, transcript, prefix, deadline):
    while True:
        for line in transcript:
            if line.startswith(prefix):
                return line
        try:
            line = output.get(timeout=max(0, deadline - time.monotonic()))
        except queue.Empty as error:
            raise AssertionError(f"Timed out waiting for {prefix!r}: {transcript}") from error
        assert line is not None, f"Server exited while waiting for {prefix!r}: {transcript}"
        transcript.append(line)


@contextlib.contextmanager
def lagged_endpoint(server, *, ready_release=None, held_snapshots=None):
    """Each client gets a separate source endpoint and a deterministic impairer."""
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as relay:
        relay.bind(("127.0.0.1", 0))
        relay.settimeout(0.005)
        stop = threading.Event()
        admitted = threading.Event()
        received = [0, 0]
        dropped = [0, 0]
        errors = []

        def forward():
            client = None
            withheld_ready = False
            pending = []
            ordering = 0
            try:
                while not stop.is_set():
                    while pending and pending[0][0] <= time.monotonic():
                        _, _, destination, payload = heapq.heappop(pending)
                        relay.sendto(payload, destination)
                    try:
                        payload, source = relay.recvfrom(4096)
                    except (socket.timeout, ConnectionResetError):
                        continue
                    if source == server:
                        if client is None:
                            continue
                        direction, destination = 1, client
                    else:
                        if client is None:
                            client = source
                        if source != client:
                            continue
                        direction, destination = 0, server
                    received[direction] += 1
                    if ready_release is not None and not ready_release.is_set():
                        if direction == 0 and payload.startswith(READY_HEADER):
                            withheld_ready = True
                            continue
                        if direction == 1 and withheld_ready and len(payload) >= SNAPSHOT_HEADER.size:
                            version, kind, token, _, phase, interrupted, tick, remaining = SNAPSHOT_HEADER.unpack_from(payload)
                            if version == MATCH_PROTOCOL and kind == 5 and token:
                                held_snapshots.put((phase, interrupted, tick, remaining))
                    if received[direction] % (11 if direction == 0 else 13) == 0:
                        dropped[direction] += 1
                        continue
                    # The only lobby reply larger than the handshake is an authenticated snapshot.
                    if direction == 1 and len(payload) > 64:
                        admitted.set()
                    delay = 0.030 + (received[direction] % 4) * 0.002
                    ordering += 1
                    heapq.heappush(pending, (time.monotonic() + delay, ordering, destination, payload))
            except OSError as error:
                errors.append(error)

        worker = threading.Thread(target=forward, daemon=True)
        worker.start()
        try:
            yield relay.getsockname()[1], dropped, admitted
        finally:
            stop.set()
            worker.join(timeout=2)
        assert not worker.is_alive(), "UDP relay failed to stop"
        assert not errors, errors


def play_round(executable, server, player_count, rules, deadline, impair=False):
    with contextlib.ExitStack() as cleanup:
        processes = []
        dropped_counts = []
        ready_release = threading.Event()
        held_snapshots = queue.Queue()
        for index in range(player_count):
            port = server[1]
            if impair:
                hold = index == 1
                port, dropped, admitted = cleanup.enter_context(lagged_endpoint(
                    server, ready_release=ready_release if hold else None, held_snapshots=held_snapshots
                ))
                dropped_counts.append(dropped)
            report = cleanup.enter_context(tempfile.TemporaryFile(mode="w+", encoding="utf-8", errors="replace"))
            probe = subprocess.Popen(
                [executable, f"--match-probe={server[0]}:{port}"],
                stdout=report,
                stderr=subprocess.STDOUT,
            )
            cleanup.callback(stop_process, probe)
            processes.append((probe, report))
            if impair and index == 0:
                assert admitted.wait(5), "First duel client did not receive a lobby snapshot"
                # Hold one admitted player in the lobby beyond the eight-second peer timeout.
                time.sleep(8.5)
            if impair and index == 1:
                # These snapshots are observed only after the admitted peer's Ready was withheld.
                ready_deadline = time.monotonic() + 6
                for _ in range(3):
                    try:
                        state = held_snapshots.get(timeout=max(0, ready_deadline - time.monotonic()))
                    except queue.Empty as error:
                        raise AssertionError("No Waiting snapshots after withholding Ready") from error
                    assert state == (0, 0, 0, DUEL_SECONDS * 60), state
                ready_release.set()
                print("MATCH_READY_GATE waiting_snapshots=3 tick=0", flush=True)

        slots = []
        results = []
        for probe, report in processes:
            try:
                code = probe.wait(timeout=max(0, deadline - time.monotonic()))
            except subprocess.TimeoutExpired:
                report.seek(0)
                print(report.read(), end="", flush=True)
                raise
            report.seek(0)
            lines = report.read().splitlines()
            assert code == 0, (probe.pid, code, lines)
            assert f"MATCH_RULES {rules}" in lines, lines
            slot_lines = [line for line in lines if line.startswith("MATCH_SLOT ")]
            round_results = [line for line in lines if line.startswith("MATCH_RESULT ")]
            assert len(slot_lines) == len(round_results) == 1, lines
            slots.append(int(slot_lines[0].split()[1]))
            results.append(round_results[0])

        assert sorted(slots) == list(range(player_count)), slots
        if impair:
            assert slots[0] == 0, "First duel client lost its slot while waiting"
        assert len(set(results)) == 1, results
        for dropped in dropped_counts:
            assert all(dropped), f"Expected losses in both directions, observed {dropped}"
        if dropped_counts:
            print(f"MATCH_RELAY_DROPS {dropped_counts}", flush=True)
        print(f"MATCH_PROBE_CONSENSUS players={player_count}", flush=True)
        return results[0]


def match_result(line, players):
    fields = line.split()[1:]
    assert len(fields) == 2 + 10 * players, line
    tick, remaining = map(int, fields[:2])
    acknowledgements = [int(fields[2 + 8 * slot + 4]) for slot in range(players)]
    assert tick > 0 and all(ack > 0 for ack in acknowledgements), line
    return tick, remaining


def match_stats(line, ticks, players):
    stats = dict(field.split("=", 1) for field in line.split()[1:])
    assert int(stats["ticks"]) == ticks, stats
    assert 0 < int(stats["max_datagram"]) <= 1200, stats
    assert int(stats["snapshot_bytes"]) > int(stats["datagrams"]) > 0, stats
    assert 0 <= float(stats["avg_us"]) <= float(stats["max_us"]), stats
    assert 0 < float(stats["p95_us"]) <= float(stats["max_us"]) + 100, stats
    assert 0 <= int(stats["over_budget"]) <= ticks, stats
    payload_per_client = int(stats["snapshot_bytes"]) * 60 / (ticks * players)
    print(f"MATCH_PROFILE players={players} payload_bytes_per_client_second={payload_per_client:.0f}", flush=True)


def check_invalid_cli(executable):
    cases = [
        ["--match-server=0", "--round=dev-range,duel,2,10,2"],
        ["--match-server=0", "--round=outer-facility,tdm,7,10,2"],
        ["--match-server=0", "--bind=300.0.0.1"],
        ["--match-probe=127.0.0.1:0"],
    ]
    for flags in cases:
        result = subprocess.run(
            [executable, *flags], capture_output=True, text=True, timeout=5, check=False
        )
        assert result.returncode == 2 and "MATCH_LISTEN " not in result.stdout, (flags, result)


def main():
    executable = sys.argv[1]
    check_invalid_cli(executable)
    with contextlib.ExitStack() as cleanup:
        server = subprocess.Popen(
            [
                executable,
                "--match-server=0",
                "--bind=127.0.0.1",
                "--round=dev-range,tdm,6,10000,2",
                f"--round=outer-facility,duel,1,2,{DUEL_SECONDS}",
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        cleanup.callback(server.stdout.close)
        cleanup.callback(stop_process, server)
        output = queue.Queue()
        reader = threading.Thread(target=server_lines, args=(server.stdout, output), daemon=True)
        reader.start()
        deadline = time.monotonic() + 50
        transcript = []
        listening = wait_for_line(output, transcript, "MATCH_LISTEN ", min(deadline, time.monotonic() + 10))
        endpoint = ("127.0.0.1", int(listening.split()[1]))

        for rules, players, impair in [("dev-range tdm 6", 12, False), ("outer-facility duel 1", 2, True)]:
            line = wait_for_line(output, transcript, "MATCH_ROUND ", deadline)
            assert line == f"MATCH_ROUND {rules}", transcript
            result = play_round(executable, endpoint, players, rules, deadline, impair=impair)
            assert result == wait_for_line(output, transcript, "MATCH_RESULT ", deadline), transcript
            tick, remaining = match_result(result, players)
            match_stats(wait_for_line(output, transcript, "MATCH_STATS ", deadline), tick, players)
            if impair:
                fields = result.split()
                assert remaining > 0 and int(fields[3]) == 2 and int(fields[6]) > 0, result
            else:
                assert tick == 120 and remaining == 0, result
            transcript.clear()

        assert server.poll() is None, "Hosted server exited after its configured rounds"


if __name__ == "__main__":
    main()
