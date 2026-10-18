# Recovery roadmap

Use bounded PRs in this order. Historical roadmaps are reference; PROGRESS.md
records completed work and exact engine revisions. No automatic merges.

| Step | Acceptance criteria |
| --- | --- |
| 0a Rules | Replace feature-expansion priorities while preserving engine/game ownership and working behavior. |
| 0b CI baseline | Pin NovaCore to a full commit SHA; Windows/MSVC and Ubuntu/Clang configure/build; movement, weapon, input, assets and network tests plus real headless boot run. GPU tests cannot pass without an active Vulkan backend. |
| 1 Test/assets | Classify existing tests; establish axes/units/transform/socket contract from code; validate one representative weapon including view, ADS, muzzle/grips, scale and animation before removing any fix. Document every deleted test's contract/replacement. |
| 2 Architecture | Extract one GameApp responsibility per change; normal match simulation/render/HUD no longer depends on Dev Range drills or telemetry. |
| 3 Vertical slice | Complete the two-player match criteria below using existing engine primitives. |
| 4 Engine support | Only necessary, separately validated NovaCore changes; record cross-repo revisions and PR dependencies. |
| 5 Hygiene | Stage required runtime content, retain recoverable source assets, remove proven obsolete code/docs, then revalidate both repos. |

## Two-player match acceptance

Two real clients can connect to one authoritative server, enter the same arena,
move/collide, fire one primary and one sidearm, deal server-validated damage,
die and respawn. Both clients agree on health, ammo, score, timer and match end.
Minimal HUD shows health/ammo/crosshair/score/timer; hit/audio feedback is usable.
A reproducible complete-match test and a manual two-player playthrough pass.
These are targets, not claims about the current implementation.

Defer matchmaking, accounts, progression, extra modes/maps/rosters, broad menu
work and speculative network/rendering expansion. Preserve existing movement.

## Ownership

NovaCore owns generic runtime, import, GPU resources, ECS, collision/KCC and
network primitives. Nemisis owns movement policy/feel, assets/sockets/mounts,
weapons/damage/TTK, scoring/spawns, match replication, maps and gameplay UI.
Cook canonical coordinates once offline; runtime mounting uses sockets and art
poses. No alignment deletion before representative-asset and playable validation.
