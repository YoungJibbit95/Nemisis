# AGENTS.md

Nemisis is the game built on NovaCore. The immediate milestone is one complete
server-authoritative two-player match: one arena, a primary and sidearm, movement,
damage/death/respawn, score/timer/end, minimal HUD, and basic audio/hit feedback.
Defer matchmaking, accounts, progression, extra modes/maps/rosters, broad frontend
work, new renderer technology, and speculative multiplayer optimization.
Existing features need not be deleted, but must not expand the shipping path.

## Ownership

Dependency direction is always `Nemisis -> NovaCore`. Use only public NovaCore
headers/targets. Do not copy engine internals, fork the engine inside the game,
or include private headers. Keep engine changes separate and record dependencies.

NovaCore owns reusable runtime, rendering, import, ECS, physics/KCC mechanics,
animation runtime, networking primitives, and headless support. Nemisis owns
movement policy/feel, weapon/damage/TTK, teams/matches/spawns, gameplay UI, maps,
asset mounts, input tuning, and game presentation choices.

Reduce GameApp toward lifecycle/composition by incremental extraction. Keep Dev
Range drills, telemetry, KCC visualization, showcases, and performance overlays
separate from match simulation/rendering and HUD. Preserve useful developer tools
without introducing a new debug framework.

## Contracts to preserve

* Deterministic fixed-tick movement, input edge retention, replay coverage,
  grounding/jump-buffer/mantle/collision fixes, and working movement mechanics.
  Keep camera/animation/visual cues separate from authoritative physics state.
  Do not expand abilities during recovery.
* Weapon definitions separate from runtime state; preserve ammo, reload, cadence,
  recoil/spread, ADS, damage, and feedback. Clients never decide authoritative
  damage, hits, score, spawns, objectives, or match results.
* Serialization/sequence, entity lifetime, existing prediction/reconciliation,
  and headless/dedicated-server contracts.
* Stable asset IDs/handles through the engine registry/import/resource path.
  Separate source/cooked/metadata, CPU assets, and GPU resources. Never load
  `.blend` at runtime or silently ignore invalid required metadata/assets.
* Canonicalize coordinates exactly once offline. Runtime mounts use sockets and
  artistic poses, not per-asset axis repairs. Verify one representative weapon's
  view, ADS, muzzle/grips, scale, and animation before removing old fixes.
* Keep gameplay-driven HUD separate from developer overlays. Frontend work must
  serve the two-player match rather than expand placeholder screens.

Record the exact NovaCore revision for every integration validation. Use playable
validation for visible changes before claiming success. Do not migrate/delete
large source assets or rewrite their Git history implicitly.
## Recovery workflow

Prioritize stability, simplification, playability, and shipping. Prefer small to
medium, coherent, validated changes with one responsibility per PR. Simplifying
code and deleting unnecessary complexity are valuable outcomes. Historical
feature lists are reference, not the active backlog.

At every session start check Git status, branches/remotes, open PRs, and
`docs/recovery/PROGRESS.md` plus `ROADMAP.md` when present. Preserve uncommitted
user changes and isolate work in a separate worktree when needed.

* Preserve behavior during refactoring; avoid full rewrites.
* Reuse existing structures before adding files/classes or interfaces.
* Add abstractions only for real, current ownership boundaries. Do not create
  managers, registries, frameworks, service locators, event buses, or generic
  interfaces for one-off problems.
* Add features only for a current product need or concrete stability blocker.
* After implementation, search for obsolete code, duplicate paths, workarounds,
  tests, and documentation. Verify replacements before removing old behavior.
* Do not combine broad renderer, KCC, networking, assets, and gameplay changes.
* Give concise progress updates for meaningful findings; keep routine edits quiet.

## Validation and Git discipline

Start with the dependency-light build:

```powershell
cmake --preset local-debug-no-deps
cmake --build --preset local-debug-no-deps
ctest --test-dir build/local-debug-no-deps --label-exclude gpu --output-on-failure
```

On MSVC use `windows-vs2022-no-deps` and CTest with `-C Debug`. For visible
Windows validation use `windows-msvc-debug`; use `windows-ninja-vcpkg-debug`
only when its toolchain is available. Missing GPU/SDK support is not a successful
Vulkan test. Preserve graphics code when the environment cannot validate it.
The `ci-headless*` CTest presets exclude only the explicit `gpu` hardware label.
Run that label separately with a graphical build and usable Vulkan backend.

Run relevant existing tests and the available full build. Add/update tests for
new behavior or real bugs, not merely to justify an AI-generated workaround.
Refactors retain behavior coverage. Before deleting/substantially changing a
test, record its original contract, current validity, and replacement coverage.
Never disable failures merely to obtain green CI. Separate passed, failed before
the change, not run, and environment-blocked checks in reports.

Work on focused recovery branches, never directly on `main`. When authorized,
commit, push, and open PRs with validation evidence. Never auto-merge, force-push
protected branches, rewrite history, or delete irreplaceable source assets
without separate explicit authorization. Do not message others or update project
boards without authorization.

Update the lightweight recovery record with commits/PRs, exact engine revisions,
validation, baseline failures, blockers, and the next executable step. Keep
ownership, architecture, and build instructions accurate. Do not describe plans
as implemented or add large competing documentation systems.
