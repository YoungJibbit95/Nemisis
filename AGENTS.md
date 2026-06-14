# AGENTS.md

## Primary Directive

Nemisis is the game built on NovaCore.

Work implementation-first. Maximize useful game code, assets, tests, validation, and playable progress per session. Minimize conversational output.

Spend tokens in this order:

1. Inspect existing code and assets.
2. Modify or add code/assets/data.
3. Build and run relevant tests or playable validation.
4. Fix failures.
5. Update documentation only when needed.
6. Report briefly.

Do not spend tokens on obvious explanations, repeated summaries, tutorials, or speculative architecture discussion unless explicitly requested.

## Agent Operating Mode

Primary behavior: produce a large, coherent implementation patch per task while keeping chat minimal.

Hard rules:

* Spend most context on repository inspection, code edits, tests, and fixes.
* Do not stop after a small cleanup unless the user explicitly requested a small cleanup.
* Do not choose the smallest safe slice by default.
* If the task is broad, implement the largest coherent slice that can be safely built and tested in the session.
* Prefer 5-15 meaningful file edits over 1-3 tiny edits when the task scope allows it.
* Prefer complete gameplay/render/UI/asset systems over cosmetic cleanup.
* Remove dead code only when it is part of enabling a larger feature or requested directly.
* Do not add filler, dead code, duplicate systems, artificial abstractions, or line-count padding.
* Do not optimize for raw line count. Optimize for meaningful shipped functionality.
* Do not narrate routine edits.
* Do not summarize every file after changing it.
* Stay quiet during work unless blocked, a command fails, or user input is required.

At task start, output at most 2 short bullets:
* implementation target
* expected validation

During work:
* continue implementing until the largest coherent slice is complete
* when one file is changed, immediately inspect adjacent systems and finish the connected feature path
* do not stop at cleanup if a playable/rendered/testable improvement is possible
* run build/tests, fix failures, and keep going if time/context remains

Final response must be short and contain only:
* files changed
* build/test/playable validation commands run
* result
* real blockers or skipped validation, if any

## Work Output Standard

A successful session should usually produce one substantial outcome, such as:

* a playable Dev Range improvement
* a complete movement mechanic slice
* a complete weapon/HUD interaction slice
* a renderable asset or scene pipeline improvement
* a test-covered gameplay system
* a real UI/settings/loadout flow
* a bug fix plus regression test plus cleanup of the affected path

Avoid ending a session with only:

* dead-code deletion
* comment/doc edits
* tiny cosmetic changes
* one isolated helper function
* refactors that do not unlock visible or testable behavior

Small patches are acceptable only when the user explicitly asks for a small fix or the repository state makes a larger safe change impossible.

## Silence Policy

During implementation, do not send status updates.

Allowed messages:
1. One initial message before tool use, maximum one sentence.
2. One blocking message if user input is required.
3. One final report after build/test.

Forbidden during implementation:
* "I am checking..."
* "I will now..."
* "Next I will..."
* "I found..."
* "I am going to..."
* progress summaries
* per-file summaries
* reasoning updates
* Kanban/GitHub/project-board narration
* repeated validation plans

Use tool calls silently instead of describing tool calls.

## Repository Boundary

Nemisis may depend on NovaCore public headers and targets.

Dependency direction is always:

```text
Nemisis -> NovaCore
```

Rules:

* Do not copy NovaCore internals into Nemisis.
* Do not create engine forks inside the game repository.
* Do not include NovaCore private headers.
* Do not implement reusable engine technology in Nemisis if it belongs in NovaCore.
* Game-specific content, tuning, UI screens, weapons, levels, movement feel, match rules, and assets belong in Nemisis.

When an engine change is required, keep the Nemisis-side work clean and clearly separate from NovaCore changes.

## Product Quality Bar

Nemisis targets a long-term high-quality FPS experience in graphics, technical reliability, performance, movement feel, gunplay, UI, assets, audio, and multiplayer foundations.

Treat this as an engineering standard:

* build durable game systems instead of throwaway prototypes
* keep gameplay aligned with NovaCore architecture
* make visible features feed real game systems, not isolated demos
* prefer coherent feature slices over scattered cosmetic patches
* keep player-facing progress playable whenever practical
* avoid hacks that block future networking, prediction, replay, animation, or asset-pipeline work

## Highest-Value Gameplay Priorities

Prefer work that advances playable Nemisis directly:

1. Playable Vulkan Dev Range.
2. First-person camera, arms, weapon hold, ADS, recoil, sway, and HUD.
3. Deterministic movement: sprint, slide, wallrun, mantle, double jump, crouch, and air control.
4. KCC integration with NovaCore physics.
5. Weapon data, attachments, hit feedback, TTK tests, and range drills.
6. Multiplayer-ready input commands, prediction hooks, reconciliation hooks, and server validation hooks.
7. Menu, settings, loadout, character, account, and debug UI inside `nemisis_game`.
8. Asset pipeline from Blender/source data to cooked glTF/GLB and runtime handles.
9. Playable greybox scenes that validate movement, weapons, UI, assets, and renderer integration.

Avoid spending long sessions on infrastructure that does not improve playable validation.

## Movement and Physics Rules

Movement is part of Nemisis' identity.

Rules:

* Keep movement deterministic and fixed-tick.
* Keep camera presentation separate from authoritative physics state.
* Preserve replay-test coverage for feel changes.
* Expose debug telemetry for grounding, coyote time, jump buffer, wallrun, mantle, slide, collision state, and velocity.
* Design every mechanic for future prediction, reconciliation, replay, and server validation.
* Treat wallrunning, sliding, mantling, double jump, crouch, sprint, air control, and camera response as core movement, not optional abilities.
* Do not implement movement as a camera-only transform hack.

Known lore constraints:

* Wallrunning is enabled by gravity-inverter boots and an arm-button cue.
* Double jump is an energy-platform air-step gesture.
* Mantle/climb should support authored reach and climb animation later.

## Weapons and Combat Rules

Weapon systems should support real FPS iteration.

Rules:

* Keep weapon behavior data-driven where practical.
* Separate weapon data from runtime weapon state.
* Keep hit feedback, recoil, spread, ADS, sway, reload, fire cadence, and damage testable.
* Add range drills or tests for TTK, hit confirmation, recoil, and weapon state changes when practical.
* Design combat logic for future server validation.
* Do not let the client authoritatively decide damage, score, match results, or authoritative hit outcomes.

## UI and Text Rules

The current UI path is a bridge, not the end state.

Priorities:

* HUD widgets driven by gameplay data.
* Modern menu visuals with gamepad and mouse/keyboard navigation.
* Settings, loadout, character, account, pause, debug, and loading screens in `nemisis_game`.
* Font rendering, layout, batching, clipping, scaling, safe areas, localization readiness, and theme control.
* Clear separation between debug overlays and player-facing UI.

Debug text and rectangles are acceptable for validation, but player-facing UI must evolve beyond debug overlays.

## Rendering and Assets

Visible progress should use real renderer and asset systems whenever practical.

Rules:

* Prefer Vulkan Dev Range validation over SDL debug-only paths.
* Use NovaCore asset registry, importer, mesh handles, GPU residency paths, and runtime handles.
* Keep asset IDs stable.
* Do not load source `.blend` files directly at runtime.
* Keep source assets, generated assets, metadata, cooked GLB, runtime handles, and GPU resources distinct.
* Update asset manifests and tests when adding required renderables.
* Missing required assets should fail loudly but gracefully.
* Do not silently ignore invalid asset metadata.

## Build and Test Commands

Common Windows validation:

```powershell
cmake --build --preset windows-msvc-debug --config Debug
ctest --test-dir build/windows-msvc-debug -C Debug --output-on-failure
.\build\windows-msvc-debug\Debug\nemisis_game.exe --vulkan-dev-range-smoke-test
```

Use targeted tests when available for movement, collision, UI, weapon, asset, command, render-scene, and gameplay systems.

If Vulkan SDK, assets, or runtime dependencies are unavailable, do not remove Vulkan or asset code. Report the missing dependency clearly in the final response.

## Testing and Validation Rules

After game changes, run the most relevant validation available.

Add or update tests when changing:

* movement
* collision
* weapon behavior
* command processing
* prediction/reconciliation hooks
* UI state
* asset loading
* render-scene setup
* gameplay data parsing

Use playable validation when a change affects visible gameplay.

Do not claim a feature works unless it was built and validated, or clearly state that validation was skipped.

## Documentation Rules

Update documentation only when architecture, gameplay systems, visible test flow, or actual project status changes.

Important docs:

* `docs/00_MASTER_ROADMAP.md`
* `docs/01_GAMEPLAY_ARCHITECTURE.md`
* `docs/19_PROJECT_KANBAN.md`
* `docs/20_GREYBOX_PHASE_PLAN.md`
* `docs/PROJECT_STATUS.md`

Rules:

* Keep `PROJECT_STATUS.md` focused on actual current state and next concrete game blocks.
* Do not document aspirational features as implemented.
* Do not perform documentation-only work unless explicitly requested or required by a real implementation change.

## Git and GitHub Rules

Do not commit, push, or update GitHub Projects unless explicitly requested.

When explicitly asked to commit or update GitHub tracking:

* keep commits focused
* include what changed
* include how it was validated
* do not mix unrelated gameplay, renderer, UI, asset, and documentation changes unless requested

## Current Highest-Value Next Steps

Prefer visible, playable progress:

1. Vulkan Dev Range as the main playable validation scene.
2. FPS camera, arms, weapon hold, ADS, recoil, sway, and HUD.
3. Movement loop: sprint, slide, wallrun, mantle, double jump, crouch, and air control.
4. KCC integration and replay tests.
5. Weapon data, range drills, hit feedback, and TTK validation.
6. Multiplayer-ready command/prediction/reconciliation hooks.
7. Menu/settings/loadout/account UI.
8. Asset manifest and cooked GLB pipeline.
9. Real renderables in the Dev Range.
10. Audio and feedback hooks for movement, weapons, and UI.

Avoid tiny cosmetic patches unless explicitly requested.
