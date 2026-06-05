# Nemisis Project Status

## Current Foundation

- Game repository consumes NovaCore through `Novacore::Engine`.
- `main.cpp` is thin and delegates to `nemisis::game::GameApp`.
- Game modules exist for input, player commands, movement, and weapons.
- Movement supports sprint, tactical sprint tuning, jump, double jump, dash, slide, and future wall-run values.
- Weapon registry supports AR, SMG, shotgun, and sidearm definitions.
- Input actions are translated into `PlayerInputCommand` for fixed-tick gameplay.
- Default input bindings cover MKB plus controller stick/buttons/triggers.
- Weapon runtime state supports ammo, shot index, fire cooldown, reload timer, and dry fire.
- Local player spawning creates a NovaCore ECS entity with identity, local-input, network, loadout, movement, and weapon runtime components.
- A tick-ordered `PlayerCommandQueue` keeps unacknowledged local commands for future server reconciliation.
- The playable dev sandbox updates actions from NovaCore window input snapshots.
- Dev telemetry logs movement, view, weapon, target, pending command, and shot trace data while the game runs.
- Mouse and right-stick look update player yaw/pitch.
- Movement is camera-relative through `PlayerViewComponent`.
- A debug target dummy supports first hitscan damage, health, hit count, elimination, and auto-respawn.
- Deterministic `ShotTraceResult` data now carries seed, direction, range, spread, and damage for future hitscan validation.
- Client command packets and server acknowledgement packets have a first deterministic binary format.
- The dev client now exercises a loopback command bridge that serializes pending commands, processes them through a server-side handoff, and acknowledges them back into the local command queue.
- Asset production has an agent-ready Blender handoff plan and initial briefs for target, weapon, arena, arms, and player proxy assets.
- Blender automation is prepared for first dev primitives through `tools/blender/make_dev_primitives.py`.
- Runtime asset ids are declared in `configs/assets/nemisis_assets.json`.
- `GameAssetCatalog` loads the game asset manifest through NovaCore's asset registry backbone and queues dev-sandbox preload requests.
- Barebones runtime menu exists with Main Menu, Dev Shooting Range, TDM placeholder, and Control placeholder screens.
- Debug UI overlay is visible through NovaCore SDL debug render primitives when SDL3 is available.
- Debug UI now has Gameplay, Network, and Assets pages, toggled with Tab or controller Start/Menu.
- The dev range requests relative mouse mode through NovaCore while menus keep normal cursor behavior.
- Local player spawning includes a health component for future authoritative damage and respawn flow.
- Weapon metrics estimate damage band, shots-to-eliminate, and measured TTK for 150 HP balance targets.
- `docs/19_PROJECT_KANBAN.md` tracks completed, doing, next, and blocked work until GitHub Projects access is available.
- Movement replay tests cover sprint distance, jump/double-jump, dash cooldown, and config-driven tuning.
- Input command, weapon simulation, weapon shot, player view, debug target, dev sandbox, player spawn, command queue, command message, and loopback bridge tests cover the newest gameplay bridge.

## Added In Latest Block

- Added `CommandPacket` and `CommandAck` serialization for the first client/server command protocol.
- Added `LoopbackCommandBridge` to send pending local commands through a NovaCore loopback channel and trim acknowledged commands.
- `GameApp::onFixedTick` now exercises the command send/server-process/client-ack path every fixed tick.
- Dev sandbox telemetry now reports sent command packets, received acknowledgements, and last acknowledged tick.
- Added `nemisis_command_messages_tests` and `nemisis_loopback_command_bridge_tests` CMake targets.
- Added `docs/16_ASSET_PRODUCTION_PLAN.md` plus initial Blender-agent briefs under `assets/briefs`.
- Added a Blender-only dev primitive generator script for future target dummy, AR blockout, and movement arena kit exports.
- Added game-side asset catalog loading, dev-sandbox streaming-zone construction, and `nemisis_asset_catalog_tests`.
- Added Blender agent instructions, job template, and PowerShell helper script.
- Added `docs/17_IDE_TOOLCHAIN_AND_BLENDER_RUNBOOK.md` plus Visual Studio 2022 no-deps preset.
- Added runtime data copy support and Visual Studio debugger working directory for `nemisis_game`.
- Added `nemisis_game --smoke-test` and CTest registration for quick runtime launch checks.
- Fixed `windows-msvc-debug` so VSCode/Visual Studio no longer need Ninja or an unset `VCPKG_ROOT`.
- Added visible main menu and mode selection state through `GameMenu`.
- Added menu/debug render commands for on-screen dev range target, target HP, renderer backend, tick, ack, asset queue, and ammo.
- Added menu actions for keyboard/controller navigation and `nemisis_game_menu_tests`.
- Moved command packet byte IO onto NovaCore `PacketWriter`/`PacketReader`.
- Added `PlayerHealthComponent`, `applyDamage`, reset/alive helpers, and `nemisis_player_health_tests`.
- Added `WeaponMetrics` with range bands, damage lookup, shots-to-eliminate, and TTK estimates.
- Added `nemisis_weapon_metrics_tests` for AR/SMG 150 HP TTK baselines.
- Added debug page cycling and relative mouse mode activation for the playable dev range.

## Next Game Blocks

- Add configurable MKB/controller sensitivity loading and response curves.
- Wire player health into hit resolution, HUD health, respawn, and server validation.
- Expand debug UI pages with frame timings, entity counts, packet loss simulation, and reconciliation error.
- Run the prepared Blender dev primitive generator once Blender is installed or exposed through tooling.
- Add first glTF metadata/import handoff from NovaCore asset catalog into renderer mesh handles.
