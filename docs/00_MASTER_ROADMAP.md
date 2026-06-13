# 00 Master Roadmap

## Vision

Nemisis is the game layer for a custom commercial-ready FPS project built on the NovaCore Engine. It uses modern references for feel, not as code bases:

- Titanfall/Apex-like momentum and skill expression.
- BO-like readable gunfights and short-to-mid TTK.
- Controller and MKB both treated as primary competitive inputs.
- Custom C++23 NovaCore engine with Vulkan-first rendering.
- No Source, Unity, Unreal, Godot, or other game engine base.

The long-term bar is an AAA-quality FPS in graphics, technical reliability, performance, movement feel, gunplay, UI, and multiplayer foundations. This roadmap should therefore favor durable engine/game architecture over throwaway prototypes while still keeping the game runnable and testable at every stage.

The first milestone is not a polished shooter. It is a game spine where Nemisis consumes NovaCore cleanly, boots a sandbox client, creates game-owned systems around engine APIs, and keeps game data separate from engine/server runtime defaults.

## Non-Negotiable Criteria

- Ground-up engine ownership through the separate NovaCore repository.
- Cross-platform architecture.
- Windows/Linux first-class; macOS later through MoltenVK.
- Low-level dependencies are allowed only when they do not become a game engine.
- Commercial licensing must be possible.
- The server is authoritative through NovaCore.
- Dedicated and listen server use NovaCore's shared simulation code.
- Large-map streaming is architected early, but first playable map remains controlled.
- Hot-reload data drives tuning.
- MKB and controller support are first-class.
- Rendering, material, lighting, animation, physics, movement, networking, UI, audio, asset, and scene-management decisions must be made with production-scale FPS requirements in mind.
- Visible gameplay features should use real engine/game systems rather than isolated proof-of-concept code.
- UI and text rendering must evolve toward a production-grade font/layout/rendering path, not remain a debug overlay.

## Product Shape

The game target is 6v6 casual-competitive arena FPS:

- TDM and Control first.
- Sandbox utility instead of hero kits.
- Core weapon roster first: AR, SMG, shotgun, sidearm.
- Full third-person characters are planned, but animation is phased.
- Readable realism visual direction.
- Marketplace assets are allowed only through a Blender-to-glTF cleanup path.
- Original Blender asset briefs are prepared before final content production so dev art, collision, sockets, and scale stay usable from day one.

## Milestones

### M0 - Toolchain and Repo

Goal: a clean project that can grow without reshuffling every week.

Deliverables:

- CMake project with NovaCore dependency resolution.
- Game executable.
- Thin `main.cpp` plus `nemisis::game::GameApp`.
- Directory structure for game code, assets, configs, and docs.

Definition of done:

- CMake configure works when toolchain exists.
- Game target is named and separated.
- NovaCore can be consumed as sibling checkout or installed package.

### M1 - Thin Spine

Goal: sandbox client, game app layer, ECS usage, input, and renderer calls all exist through NovaCore APIs.

Deliverables:

- NovaCore window/input backend with fallback.
- NovaCore renderer foundation with SDL debug path, opt-in Vulkan smoke backend, clear-frame intent, and null fallback.
- Game creates a sandbox camera entity through NovaCore ECS.
- Sandbox scene creates a camera entity.
- Game configs stay game-owned.

Definition of done:

- Game starts, opens a window when SDL3 exists, and runs frames.
- No engine internals are copied into Nemisis.
- Game code lives outside `main.cpp`.
- Input layer reports MKB and controller availability.

### M2 - Renderer Core

Goal: the renderer becomes a real production subsystem.

Deliverables:

- Vulkan instance/device/swapchain.
- Render pass or dynamic rendering path.
- Depth buffer.
- Mesh upload.
- Basic shader pipeline.
- Camera uniform data.
- Debug GPU markers where available.

Definition of done:

- Window clears to a configurable color.
- A simple mesh renders with depth.
- Resize/recreate path works.

### M3 - Asset Pipeline

Goal: load and render real content.

Deliverables:

- Blender-to-glTF import path.
- Game-side Blender asset production plan and handoff briefs.
- Mesh/material/texture asset handles.
- Cooked asset cache.
- Hot reload for data configs.
- Async loading interface.

Definition of done:

- A glTF scene loads.
- First dev primitives have source/export/metadata and pass naming/scale validation.
- Missing materials get deterministic fallbacks.
- Asset reload does not crash the running sandbox.

### M4 - Movement Controller

Goal: the movement simulation becomes the game's identity.

Deliverables:

- Custom capsule character controller.
- Grounding, slopes, steps, crouch, sprint.
- Slide, mantle, jump, air control.
- Advanced movement tech hooks.
- Deterministic input command recording.

Definition of done:

- Movement replay produces stable results.
- Server can validate movement envelopes.
- MKB and controller produce equivalent action data.

### M5 - Combat Prototype

Goal: turn the sandbox into a shooter.

Deliverables:

- AR, SMG, shotgun, sidearm.
- Hitscan traces.
- Recoil pattern + noise.
- ADS and spread state.
- Damage zones and hit feedback.
- Reload and ammo model.

Definition of done:

- Weapon data can be tuned without recompiling.
- TTK targets are measurable.
- A target can be damaged and killed.

### M6 - Characters and Animation

Goal: first-person and third-person representation become shippable foundations.

Deliverables:

- Skeleton import.
- Clip playback.
- Blend trees.
- Weapon sockets.
- Hitboxes driven by animation poses.
- Event tracks.

Definition of done:

- A remote player proxy animates locomotion.
- Weapon muzzle socket is usable.
- Hitboxes follow pose data.

### M7 - Netcode

Goal: multiplayer feels fair under real latency.

Deliverables:

- UDP transport.
- Sequence/ack reliability.
- Snapshot replication.
- Client prediction.
- Reconciliation.
- Interpolation for remote players.
- Server rewind for hit validation.

Definition of done:

- Movement remains responsive at simulated latency.
- Hits validate against rewound server state.
- Packet loss simulation produces recoverable state.

### M8 - UI and Settings

Goal: the game feels like a modern product shell.

Deliverables:

- Custom NanoVG-style UI renderer.
- SDF text.
- Blur/glow passes.
- Main menu, direct connect, settings.
- MKB and gamepad navigation.
- HUD, scoreboard, killfeed.

Definition of done:

- Settings persist.
- Controller can navigate menus.
- UI scales cleanly at 1080p and 1440p.

### M9 - Modes

Goal: actual match flow.

Deliverables:

- TDM.
- Control.
- Teams and spawns.
- Score rules.
- Objective capture.
- Match start/end/restart.

Definition of done:

- A complete local match can be played.
- Scoreboard and win conditions are correct.

### M10 - Performance and Polish

Goal: push toward the first real vertical slice.

Deliverables:

- 144 FPS PC target tuning.
- Profiling overlays.
- Audio events.
- Weapon effects.
- Visual settings.
- Network stats.

Definition of done:

- Frame timing is tracked.
- Bottlenecks are visible.
- The slice is playable and tunable.

## Major Risks

- Full custom animation plus advanced movement plus netcode is a large scope.
- Vulkan setup can consume time before gameplay exists.
- Controller aim assist must be measured carefully to avoid unfairness.
- Marketplace assets may require cleanup and licensing review.
- Large-map streaming can distract from first playable arena if overbuilt.

## Scope Guardrails

Do first:

- NovaCore dependency integration.
- Thin client/game spine.
- Basic renderer usage.
- Input/action config.
- Movement commands.
- Weapon data model.

Do not do first:

- Battle royale.
- Ranked matchmaking.
- Anti-cheat.
- Full editor.
- Motion matching.
- Massive map content.
- Photoreal asset production.







