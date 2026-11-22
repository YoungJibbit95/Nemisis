# Hosting a Nemisis match

Build Nemisis against the NovaCore revision in `cmake/novacore-revision.txt`. The
`nemisis_match_server` target runs the authoritative simulation without a game
window, GPU initialization or game asset directory. Build `nemisis_game` with
the usual graphical preset for players. Use the same protocol build on both ends.

## Start a server

This PowerShell example runs from the Nemisis checkout after building the
Windows dependency-light Release server.

```powershell
.\build\windows-vs2022-no-deps\Release\nemisis_match_server.exe --bind=0.0.0.0 --match-server=27015 --round=outer-facility,duel,1,10,180 --round=dev-range,tdm,6,30,300
```

Each `--round` specifies the map, mode, players per team, score limit and round
duration in seconds, in that order. Both `outer-facility` and `dev-range`
support `duel` (1v1) and `tdm` (one to six players per team). The score limit
accepts 1–10000 and duration accepts 1–3600 seconds. A round starts when all
slots are occupied. Join order alternates teams. A kill awards one point. At
the score or time limit the server sends the final scores, clears the sessions
and opens the next lobby on the same UDP port. The rotation repeats. Players
rejoin to enter the next round. An active player timing out after eight seconds
interrupts the round. A waiting player timing out releases their slot.

With no arguments the dedicated executable serves an Outer Facility duel on
`127.0.0.1:27015`. Set `--bind` to the desired local IPv4 address, or to
`0.0.0.0` to listen on all interfaces. Port zero selects an available port and
prints it on the `MATCH_LISTEN` line. Run one server process per port for
concurrent matches. Stop the process with Ctrl+C.

## Join from the game

Pass the server's reachable numeric IPv4 address to the graphical executable,
for example `nemisis_game.exe --join=192.168.1.20:27015`. For a server on the
same PC use `--join-local=27015`. The host chooses the map, teams, spawn
positions, damage, timer and score. Press Escape to return to the menu.

## Network behavior and measurements

The client proves that it can receive replies at its UDP endpoint, then uses the
issued session token for inputs and snapshots. Each input has a sequence number
and the snapshot tick the player saw. The server consumes at most one input per
player per fixed 60 Hz tick. The client predicts local movement and weapon
state, restores the acknowledged authoritative state, then replays pending inputs.
Expired or future shot timestamps use current server poses. Rewind covers up to
12 ticks (200 ms), preserves static cover and checks which life a target was in.

During play the server normally sends 30 snapshots per second. Each includes the
recipient's full prediction baseline and compact state for the other players.
`MATCH_STATS` reports host work per tick in microseconds (including receive,
simulation, encoding and sending), ticks over 16.67 ms, outgoing snapshot
payload bytes, packet count and largest payload. It excludes scheduled sleep
and IP/UDP headers. Percentiles use 100 µs histogram buckets.

From the Nemisis directory, run the regression suite after building the
game, dedicated server and tests.

```powershell
ctest --test-dir build/windows-vs2022-no-deps -C Debug -R 'hosted_match|local_facility_duel|match_rules_tests|match_codec_tests' --output-on-failure
```

The hosted process test fills both six-player teams, checks the timed result
on every client, then plays the next map through a UDP relay with repeatable
delay and packet loss. The rules test covers movement during rewind, static
cover, friendly blocking and respawn boundaries.
