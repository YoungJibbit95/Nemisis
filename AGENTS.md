# AGENTS.md

## Primary Directive

Nemisis is the game built on NovaCore. Work implementation-first, keep explanations concise, and spend most effort on architecture, code, tests, assets, and playable validation.

## Product Quality Bar

Nemisis targets a long-term AAA-quality FPS experience in graphics, technical reliability, performance, movement feel, gunplay, UI, and multiplayer foundations. This is a practical engineering standard:

* build durable systems instead of throwaway prototypes
* keep gameplay tightly aligned with NovaCore's engine architecture
* prioritize renderer, material, lighting, animation, physics, movement, input, networking, UI, audio, assets, and scene-management foundations
* make each visible game feature feed real engine/game systems rather than isolated demos
* avoid small fragmented changes when a coherent larger slice can be implemented safely

## Repository Boundary

Nemisis may depend on NovaCore public headers and targets.

Dependency direction is always:

```text
Nemisis -> NovaCore
```

Nemisis must not copy NovaCore internals or create engine forks inside the game repository.

## Gameplay Priorities

Highest-value work:

* playable Vulkan Dev Range
* first-person camera, arms, weapon hold, ADS, recoil, sway, and HUD
* deterministic movement: sprint, slide, wallrun, mantle, double jump, crouch, air control
* robust KCC integration with NovaCore physics
* weapon data, attachments, hit feedback, TTK tests, and range drills
* multiplayer-ready command, prediction, reconciliation, and server validation hooks
* menu/settings/loadout/account UI inside `nemisis_game`
* asset pipeline from Blender/source data to cooked glTF and runtime handles

## UI and Text Rules

The current UI path is a bridge, not the end state.

Goals:

* production-quality font rendering and layout
* modern menu visuals with gamepad and MKB navigation
* HUD widgets driven by gameplay data
* loading screens, settings, loadout, character, account, and debug pages in the main executable
* eventual batching, clipping, scaling, safe areas, localization readiness, and theme control

Debug overlays are useful, but the main UI must evolve beyond debug text and rectangles.

## Movement and Physics Rules

Movement is part of Nemisis' identity.

Rules:

* keep movement deterministic and fixed-tick
* preserve replay-test coverage for feel changes
* keep camera presentation separate from authoritative physics state
* expose debug telemetry for grounding, coyote time, jump buffer, wallrun, mantle, slide, and collision state
* design every mechanic for future server validation
* treat wallrunning, sliding, mantling, and double jump as core movement, not abilities

Known lore constraints:

* wallrunning is enabled by gravity-inverter boots and an arm-button cue
* double jump is an energy-platform air step gesture
* mantle/climb should support authored reach and climb animation later

## Rendering and Assets

Visible progress should use real renderer and asset systems whenever practical.

Rules:

* prefer Vulkan Dev Range validation over SDL debug paths
* keep asset ids stable
* use NovaCore asset registry, importer, mesh handles, and GPU residency paths
* do not load source `.blend` files directly at runtime
* keep source assets, generated assets, metadata, cooked GLB, and runtime handles distinct
* update asset manifests and tests when adding required renderables

## Testing and Validation

After game changes, run relevant tests.

Common commands:

```powershell
cmake --build --preset windows-msvc-debug --config Debug
ctest --test-dir build/windows-msvc-debug -C Debug --output-on-failure
.\build\windows-msvc-debug\Debug\nemisis_game.exe --vulkan-dev-range-smoke-test
```

Add or update tests for movement, collision, UI, weapon, asset, command, and render-scene changes.

## Documentation and GitHub Tracking

When architecture, gameplay systems, or visible test flow changes, update the relevant docs:

* `docs/00_MASTER_ROADMAP.md`
* `docs/01_GAMEPLAY_ARCHITECTURE.md`
* `docs/19_PROJECT_KANBAN.md`
* `docs/20_GREYBOX_PHASE_PLAN.md`
* `docs/PROJECT_STATUS.md`

For large completed steps, commit, push, and update GitHub Projects with what changed and how it was validated.

