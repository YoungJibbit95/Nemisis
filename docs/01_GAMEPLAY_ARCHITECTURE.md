# 01 Gameplay Architecture

Nemisis game code is split by gameplay responsibility and consumes NovaCore through public `novacore/...` headers only.

## Architecture Goal

Nemisis is built as the first real product on NovaCore, not as a disposable demo. Gameplay systems should prove and shape the engine while remaining clean game-side code. The long-term target is an AAA-quality FPS feel, so implementation choices must support high-end rendering, polished UI, deterministic movement, weapon feel, animation, networking, assets, audio, and performance without forcing a later foundation rewrite.

Every visible game feature should answer two questions:

- Does this make the game more playable, testable, or production-ready?
- Does this use or improve the correct NovaCore foundation instead of bypassing it?

## Current Game Modules

- `nemisis::game`: app bootstrap and high-level game lifetime.
- `nemisis::dev`: playable developer sandbox telemetry, debug target dummy, and debug-state feedback.
- `nemisis::player`: normalized input commands, command queues, player ECS components, and local player spawn helpers.
- `nemisis::input`: default game action names and MKB bindings.
- `nemisis::movement`: movement tuning, state, and deterministic simulation foundation.
- `nemisis::weapons`: weapon definitions, prototype registry, and deterministic fire/reload simulation.
- `nemisis::assets`: game-owned runtime asset catalog and dev-sandbox preload zone construction.

## Direction

- MKB and controller devices produce `PlayerInputCommand`.
- NovaCore input actions map raw controls, buttons, sticks, and triggers into game actions.
- Look input updates `PlayerViewComponent` yaw/pitch.
- Movement commands are transformed relative to player view before movement simulation.
- `GameApp::onFrame` updates actions from the NovaCore window input snapshot.
- Dev sandbox telemetry exposes the first playable test loop while rendering and UI are still minimal.
- `PlayerCommandQueue` keeps local commands pending until future server acknowledgement.
- The local player is represented as a NovaCore entity with identity, ownership, network, loadout, movement, and weapon runtime components.
- Movement consumes commands and fixed delta time.
- Weapons consume commands through `FireRequest` and advance fixed-tick runtime state.
- Weapon shots produce deterministic trace data from tick seed, shot index, spread, recoil, and movement speed.
- Debug target hits apply first hitscan damage against a target sphere.
- Client command packets serialize local input commands for the first server handoff path.
- Loopback command acknowledgement trims local pending commands and mirrors the future dedicated-server path.
- The asset catalog maps Nemisis-specific asset ids onto NovaCore's manifest, registry, and streaming request backbone.
- `GameApp` advances player component state instead of loose gameplay member variables.
- Movement and weapon systems now consume NovaCore parsed config documents.
- Config reloads reapply tuning at runtime through `ConfigRegistry`.
- Game code may create NovaCore entities, but must not reach into NovaCore internals.
- Production UI, font rendering, material/lighting presentation, animation-driven first-person/third-person representation, audio feedback, and multiplayer validation are first-class roadmap systems, not optional polish.
- Movement mechanics such as wallrunning, sliding, mantling, and double jump must stay deterministic, debug-visible, and server-validation-ready from the start.

## First Acceptance

- `main.cpp` stays thin.
- `GameApp` owns game subsystem instances.
- Movement and weapon code compile as separate sources.
- Movement replay tests protect deterministic feel changes.
- Input command tests protect raw action-to-command mapping.
- Weapon simulation tests protect fire cadence, cooldown, dry fire, and reload behavior.
- Weapon shot tests protect deterministic trace generation.
- Player view tests protect camera-relative movement.
- Debug target tests protect first hitscan damage application.
- Dev sandbox tests protect telemetry summary and debug feedback.
- Player spawn tests protect expected ECS component composition.
- Command queue tests protect monotonic pending-command behavior for server handoff.
- Command message tests protect deterministic command/ack packet payloads.
- Loopback bridge tests protect pending-command acknowledgement flow.
- Asset catalog tests protect game asset manifest loading and dev-sandbox preload zone construction.
- Config files match the planned movement and weapon vocabulary.
