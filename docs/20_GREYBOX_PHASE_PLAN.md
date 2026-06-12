# 20 Greybox Phase Plan

## Decision

Camera, player, and world work should start now. It is not too early.

Nemisis needs a visible, testable FPS loop while NovaCore's Vulkan renderer, glTF importer, KCC, and netcode continue maturing. The current default is now the Vulkan 3D Dev Range; SDL debug drawing is only a legacy/debug path.

## Phase 0 - Active Now

Goal: make the game feel like a developer shooting range even before full 3D rendering.

Implemented:

- `GreyboxWorld` data model with stable primitive ids.
- Floor, side/back walls, cover, ramps, player spawn, enemy spawn, range markers, and debug target lane.
- Local player spawn pulled from world data.
- Dev sandbox sample now carries view data.
- SDL debug renderer draws a top-down map, player marker, yaw direction, target marker, and player-to-target line.
- SDL debug renderer overlays a budgeted A0 environment GLB wireframe from extracted CPU mesh positions/indices.
- A0 Blender assets generated for props, characters, weapons, and environment kit.
- A0 glTF metadata binds into NovaCore mesh handles.
- A0 GLB scene info imports into NovaCore mesh handles with mesh/node/material totals.
- A0 GLB CPU mesh data extracts into NovaCore mesh handles with primitive/vertex/index totals.
- NovaCore runtime probe detects local Vulkan runtime/device even before the compiled Vulkan backend is active.
- NovaCore compiled Vulkan backend creates a swapchain, depth buffer, and world box pipeline through `--vulkan-dev-range-smoke-test`.
- Dev Range greybox primitives are submitted as first Vulkan 3D world boxes.
- NovaCore registers extracted GLB mesh data as renderer-owned resources, queues Vulkan upload into device-local vertex/index buffers, and tracks residency/deferred destruction.
- Dev Range submits A0 arena/weapon/character assets plus generated prototype-pack assets as first in-world mesh instances.
- Prototype-pack GLBs now cover SMG, humanoid, wall, floor, crate, ramp, and target stand placeholders.
- Plain `nemisis_game` boots into the Vulkan Main Menu with `require_vulkan=true`.
- `nemisis_game --dev-range` boots directly into the Vulkan 3D Dev Range for fast iteration.
- The Dev Range and menu now both run through the same `nemisis_game` executable so gameplay, UI, loading, and smoke-test paths stay unified.
- First-person arms and a small 3D aim marker are submitted into the Vulkan frame.
- First greybox collision resolver blocks player bounds and primitive overlap.
- NovaCore `PhysicsWorld` now backs greybox collision resolution, surface metadata, wall-run contact, and wall probe reporting.
- Wall-run panel primitives are in the map and feed wall normal/tangent debug data into movement and UI.
- A2 visual-pack assets are visible in-world and include first serious weapon, operator, and map-piece proxies.

Acceptance:

- Running `nemisis_game` opens a window in SDL-enabled builds.
- Main menu can enter Dev Shooting Range.
- Dev range shows player/world/target layout and live debug telemetry.
- Tests pass, including `nemisis_greybox_world_tests`.

## Phase 1 - Next 2 To 4 Large Steps

Goal: turn the debug-map greybox into an in-world playable greybox.

Implemented foundation:

- Add glTF metadata/import shim in NovaCore.
- Add renderer mesh-handle placeholders and missing-asset diagnostics.
- Bind generated A0 `.glb` files to asset ids from `configs/assets/nemisis_assets.json`.
- Import generated A0 `.glb` scene info and surface scene totals in the Assets debug page.
- Extract generated A0 `.glb` CPU mesh data and surface primitive/vertex/index totals in the Assets debug page.
- Draw a first debug wireframe preview from extracted A0 environment GLB mesh data.
- Add KCC collision queries against greybox primitives.
- Upload extracted mesh data to the renderer.
- Submit first uploaded GLB mesh draw commands.
- Render simple floor, wall, cover, ramp, target, character, and weapon proxy meshes.

Still planned:

- Add basic material fallback selection, texture/material binding, and stronger light/debug controls.
- Replace the first static-collider KCC layer with capsule sweeps, richer depenetration ordering, timed mantle attach/climb curves, and moving-platform contacts.
- Add debug overlays for entity count, physics time, contact manifold count, wall-run eligibility, and server replay validation.

Acceptance:

- Player can move around a visible greybox arena.
- Walls, cover, ramps, floor, low steps, and ledges block or support movement.
- Debug target is represented by an in-world proxy mesh.
- First-person weapon proxy can be attached to the camera or drawn as a placeholder.

## Phase 2 - First Playable Combat Loop

Goal: convert the range into a tiny shooter test field.

Planned:

- Multiple target dummies with reset/spawn rules.
- Health and ammo HUD backed by real components.
- Weapon damage events wired through player/target health.
- Respawn/reset flow for the dev range.
- Basic latency/loss simulation UI for command bridge tests.
- Server-side validation hooks for movement and hits.

Acceptance:

- The player can spawn, move, aim, shoot, damage targets, reload, and reset the range.
- Gunfeel metrics can be tested against measured TTK.
- Movement and shooting data are visible in debug UI.

## Phase 3 - Multiplayer Greybox

Goal: make the same greybox useful for 6v6 systems.

Planned:

- TDM match lifecycle shell.
- Control objective lifecycle shell.
- Dedicated/listen server session flow.
- Snapshot interpolation for remote proxies.
- Client prediction and reconciliation error debug readouts.
- Spawn safety and objective layout tests.

Acceptance:

- Two local or LAN processes can connect to the same greybox session.
- Remote players are represented by simple proxies.
- Commands, snapshots, health, and objective state have measurable debug output.

## Current Greybox Answer

We are in Greybox Phase 0 now.

Phase 1 has crossed the first in-world asset threshold. Metadata, mesh handles, A0/A1/A2 scene-info import, CPU mesh extraction, Vulkan runtime probe, compiled Vulkan 3D primitive path, renderer-owned GLB resource upload/draw, first-person mesh proxies, world debug lines, configurable lighting/FOV tuning, NovaCore-backed ramp/step/ledge/wall-run/mantle surface queries, and A2 visual assets are in. The remaining Phase 1 pressure is capsule sweeps, timed mantle climb polish, slide validation, richer material fallback, swapchain resilience, and keeping visual assets aligned with collision truth.
