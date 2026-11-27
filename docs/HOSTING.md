# Hosting a Nemisis match

Select **Play → Private Match with Bots** for a five-minute, 20-point 2v2 match
on Outer Facility. You and one bot face two opponents. The in-process host uses
the same movement, damage, scoring and respawn rules as the dedicated server.
Escape returns to the main menu. No additional game window or server is needed.

The [Play menu](recovery/visual/private-match/menu.png) and
[Outer Facility during respawn](recovery/visual/private-match/outer-facility.png)
are 1280×720 captures of the Vulkan game at `d0e3c382bcd810bbd525cd6fdced1b8e34897707`
with engine `413f73997f6f498a6793fb999a104bd39f49c515`. The second frame
shows two independently posed opponents. This release uses the existing training
weapon and environment presentation on this host revision.

Run `nemisis_game --game --private-match-smoke` from a directory containing the
game assets and configs to repeat the menu, match and Escape paths. In the captured
run the host reached tick 541 with three posed remote operators and 53 bot hits.
Escape released all three mesh handles and the match session. The dedicated,
loopback, private-match, rules, codec and menu tests also passed.

## Dedicated host

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
accepts 1–10000 and duration accepts 1–3600 seconds. A round starts when all slots are occupied and every player has rendered the
server-selected map and reported ready. The timer remains stopped while players load. Join order alternates teams. A kill awards one point. At
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
