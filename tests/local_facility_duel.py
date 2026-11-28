"""Two independent client processes play a scored local match on an ephemeral port."""
import contextlib
import math
import re
import subprocess
import sys


def main():
    executable = sys.argv[1]
    with contextlib.ExitStack() as cleanup:
        def start(flag):
            proc = subprocess.Popen([executable, flag], text=True, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT)
            def finish():
                if proc.poll() is None:
                    proc.kill()
                proc.wait(timeout=5)
                proc.stdout.close()
            cleanup.callback(finish)
            return proc

        server = start("--match-test-server=0")
        port_line = server.stdout.readline().strip()
        assert port_line.startswith("MATCH_LISTEN "), port_line
        port = int(port_line.split()[1])
        clients = [start(f"--match-probe={port}") for _ in range(2)]
        reports = [process.communicate(timeout=30)[0] for process in [*clients, server]]
        for report in reports:
            print(report, end="")
        assert all(process.returncode == 0 for process in [*clients, server])
        results = [next(line for line in report.splitlines() if line.startswith("MATCH_RESULT "))
                   for report in reports]
        assert results[0] == results[1] == results[2], results
        # Winner, victim, ammo spent, processed command and arena position are
        # replicated separately. A pair of silent clients must fail this fixture.
        fields = list(map(float, results[0].split()[1:19]))
        weapon, rifle_ammo, other_weapon, other_sidearm = results[0].split()[19:]
        assert weapon == "sidearm_01" and 0 < int(rifle_ammo) < 30
        assert other_weapon == "ar_01" and int(other_sidearm) == 15
        assert fields[2] == 2 and fields[10] == 0 and fields[11] == 0, fields
        assert 0 < fields[4] < 15 and fields[5] >= 8 and fields[13] == 0, fields

        observed_slots = set()
        for report in reports[:2]:
            predicted = re.search(r"MATCH_PREDICT slot=(\d) input=1 z=([-\d.]+)", report)
            assert predicted, report
            slot, z = predicted.groups()
            observed_slots.add(slot)
            first_server = re.search(rf"MATCH_AUTH slot={slot} ack=1 z=([-\d.]+) server=\d+", reports[2])
            reconciled = re.search(rf"MATCH_RECONCILE slot={slot} ack=(\d+) z=([-\d.]+) server=(\d+)", report)
            assert first_server and reconciled and int(reconciled[1]) >= 1, report
            assert math.isclose(float(z), float(first_server[1]), abs_tol=0.0002)
            # Reordering or a late poll can supersede the initial ACK snapshot.
            if reconciled[1] == "1":
                same_tick = re.search(
                    rf"MATCH_AUTH slot={slot} ack=1 z=([-\d.]+) server={reconciled[3]}\b", reports[2]
                )
                if same_tick:
                    assert same_tick[1] == reconciled[2]
            assert "MATCH_RESPAWN slot=1" in report.splitlines(), report
        assert observed_slots == {"0", "1"}


if __name__ == "__main__":
    main()
