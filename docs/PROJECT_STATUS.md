# Nemisis Project Status

## Latest Block - Fixed-Tick Input, Higher Movement, Weapon Mounts, And Mesh Lighting

- Added `GameplayInputBuffer` so one-frame jump, slide, reload, pickup, and weapon-switch edges captured during render frames survive until the next fixed gameplay tick.
- Raised the base jump profile to the requested 1.75x jump-height target with `jump_velocity` 7.35, lighter gravity at -22.0, longer jump buffering, stronger double-jump impulse, and explicit `double_jump.buffer_time`.
- Split ground-jump availability from double-jump availability and added a buffered air-jump handoff so the second Space press can survive coyote/collision timing instead of being lost unless the key is spammed.
- Tightened collision handoff after jump/double-jump commands so unstable ground contacts do not immediately cancel rising jump arcs.
- Rebuilt first-person weapon presentation with per-weapon Project asset mounts: AR keeps its own imported forward correction, SMG/shotgun/sidearm no longer inherit AR yaw, long weapons use the corrected upright roll, and ADS pulls weapon placement toward the sightline.
- Added a camera-linked first-person body layer and upgraded arms placement so the view submits weapon, body, and arms mesh anchors instead of relying on standalone debug hand boxes.
- Replaced the most visible wall-run, mantle, and double-jump arm placeholder boxes with first-person arms mesh cues while retaining small VFX boxes for boots, muzzle, and energy platform feedback.
- Expanded NovaCore/Nemisis mesh lighting controls with fill, rim, specular, contrast, and saturation so imported GLBs read better while the renderer waits for full texture/material descriptor binding.
- Added regression coverage for fixed-tick input buffering, buffered double-jump reliability, per-weapon Project asset rotations, ADS sightline movement, and expanded render tuning.
- Verified `cmake --build --preset windows-msvc-debug`, Nemisis `ctest` 32/32, NovaCore build, and NovaCore smoke test 1/1.

## Latest Block - Project Asset Integration, First-Person Cleanup, And Movement Reliability

- Moved the user-provided character and weapon GLBs from `Project/Assets` into `Nemisis/assets/project_assets` and updated their metadata/catalog paths to repo-local runtime paths.
- Copied `skybox1.glb` into `assets/project_assets`, cataloged it as `env_project_skybox1`, added metadata, and rendered it as a first-pass large GLB sky environment in the Dev Range.
- Raised required dev renderables to 36 ids and regenerated `assets/processed/readiness/asset_readiness_scan.json`; there are 36 GLBs under `assets` and no orphan GLBs.
- Switched Project character usage into target lane dummies and local player body presentation while lowering/scaling the local first-person body so the camera no longer sits inside the head.
- Removed the large first-person debug weapon/hand boxes from the Dev Range view; first-person visuals now rely on the actual weapon and arms meshes plus small muzzle/aim feedback.
- Added full NovaCore `RenderMesh3D` pitch/roll support and wired Nemisis first-person weapons/arms to camera pitch, wall-run roll, recoil, reload, ADS, sway, and mantle lift.
- Enlarged and repositioned the real Project weapon models for first-person use, with stronger ADS centering and a more visible ADS FOV reduction.
- Added gameplay input and network command protocol v3 fields for weapon pickup and direct weapon switching.
- Added Dev Range weapon rack pickup pads and gameplay pickup logic: `F`/controller `Y` equips the nearest rack weapon, while `4`/`5`/`6` and controller D-pad select AR/SMG/sidearm.
- Moved controller range reset off `Y` so pickup no longer conflicts with resetting the Dev Range; keyboard reset remains `P`.
- Relaxed and stabilized wall-run entry by allowing airborne/wall-running/dashing states to claim wall contact before collision grounding cancels it, with wider probes and smoother vertical carry.
- Reordered double-jump handling so a second jump press triggers the energy-platform jump immediately after coyote/ground jump has been consumed.
- Added wall-run FOV/roll camera presentation in `PlayerCameraRig` and regression coverage for ADS FOV narrowing.
- Verified `cmake --build --preset windows-msvc-debug`, Nemisis `ctest` 32/32, NovaCore build, and NovaCore smoke test 1/1.

## Previous Block - Input Reliability, Clickable UI, And KCC Stabilization

- Added NovaCore input edge retention for short taps, plus absolute pointer position in `InputSnapshot`, so quick R/Space/C/mouse clicks are no longer lost when press/release happens inside one frame.
- Extended `PlayerInputCommand` and command packet protocol v2 with held state for jump, slide, mantle, and reload; reload now accepts held intent when a reload is legal.
- Added slide input buffering and held-slide latching so sprint-slide can be pressed slightly early, survive landing timing, and avoid repeated auto-slide spam while held.
- Fixed active mantle handoff by allowing held jump/mantle to trigger candidates and preventing normal collision resolve from overwriting the deterministic climb interpolation.
- Improved NovaCore KCC ramp grounding by treating ramp/slide surfaces as continuous walkable heightfields instead of single step edges while preserving disabled airborne step-up behavior.
- Added runtime collision proxies for visible A2 asset-stage pieces: slide ramp, wall-run panel, cover crate, range hero prop, plinths, and backboard.
- Earlier added a runtime bridge for sibling-folder Project GLBs under `../Assets`; the current path has superseded that bridge by moving the active character/weapon/skybox assets into committed repo-local `assets/project_assets`.
- Added clickable Main Menu pointer routing for tabs and selectable rows across Play, Gamemodes, Loadout, Character, Settings, and Account surfaces.
- Added asset readiness audit docs/tooling from the asset worker: Blender 5.1.2 validated 21 `.blend` sources and 28/28 GLBs; remaining asset gaps are material JSONs and authored `col_` proxies.
- Verified NovaCore Debug build + smoke CTest and Nemisis Debug build + 32/32 CTest suite, including Vulkan main menu/dev-range smoke paths.

## Previous Block - UI Backbone And Physics Contacts

- Fixed the Vulkan UI/text orientation at the NovaCore shader layer, so `nemisis_game` now gets the corrected top-left screen-space UI path through the normal Vulkan renderer.
- Expanded game-side UI infrastructure with `UiCanvas` text metrics, fit-to-width scaling, shadow/outline text, panels, buttons, pills, and dividers; Main Menu tabs, selectable rows, HUD panels, and debug panels now use the upgraded primitives.
- Reworked mantle from an instant position snap into a deterministic fixed-tick climb curve with start position, target position, progress, target normal, and grounded handoff at completion.
- Propagated NovaCore KCC contact manifolds into `GreyboxCollisionResult`, Dev Sandbox logs, Gameplay debug HUD, and Dev Range world debug lines.
- Added contact-role coverage for ground, step, wall, bounds, and sweep diagnostics so physics bugs can be debugged from live HUD/log output instead of guessing from the last collider id.
- Verified `cmake --build --preset windows-msvc-debug --config Debug`, `ctest --test-dir build\windows-msvc-debug -C Debug --output-on-failure` with 32/32 passing tests, and `nemisis_game.exe --vulkan-dev-range-smoke-test`.

## Current Foundation

- Game repository consumes NovaCore through `Novacore::Engine`.
- `main.cpp` is thin and delegates to `nemisis::game::GameApp`.
- Game modules exist for input, player commands, movement, and weapons.
- Movement supports sprint, tactical sprint tuning, jump, double jump, dash, slide, and the first wall-run contact/wall-jump flow.
- Movement includes coyote time, jump buffering, configurable wall-run probe/min-height tuning, and timed mantle exit state for the playable Dev Range KCC loop.
- Movement now carries presentation-facing tech cues for wall-run gravity activation, boot glow, double-jump energy platform, wall-jump detach, and mantle reach.
- Movement now supports the first engine-backed mantle/climb foundation: NovaCore mantle probe, ledge-top target snap, `mantle-climb` tech cue, and debug visualization.
- Weapon registry supports AR, SMG, shotgun, and sidearm definitions.
- Input actions are translated into `PlayerInputCommand` for fixed-tick gameplay.
- Default input bindings cover MKB plus controller stick/buttons/triggers.
- Weapon runtime state supports ammo, shot index, fire cooldown, reload timer, and dry fire.
- Weapon runtime state now also tracks ADS alpha, burst count, recoil pitch/yaw offsets, reload progress, and time since last shot.
- Weapon shot traces now consume runtime recoil, ADS blend, movement speed, sprinting, and airborne state for deterministic spread/recoil directions.
- Movement simulation is acceleration/friction based instead of snap-to-speed, with slide duration, slide steering, slide-jump momentum, dash duration, air drag, and horizontal speed telemetry.
- A local `PlayerCameraRig` produces smoothed render-camera position, visual pitch/yaw recoil, head bob, weapon sway offsets, roll, speed fraction, ADS fraction, and FOV kick.
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
- `DevAssetBindings` validates required dev asset ids, loads glTF metadata, imports GLB scene info, extracts CPU mesh data, and registers mesh handles through NovaCore.
- Barebones runtime menu exists with Main Menu, Dev Shooting Range, TDM placeholder, and Control placeholder screens.
- Main Menu now has Play, Gamemodes, Loadout, Character, Settings, and Account tabs.
- The normal `nemisis_game` launch starts in the Vulkan Main Menu; `--dev-range` remains the explicit fast path into the range.
- The menu owns a loading screen flow for Firing Range, TDM, and Control placeholders.
- Runtime game settings now cover mouse sensitivity, controller sensitivity, aim assist flags, damage numbers, HUD scale, and debug world-line visibility.
- Loadout state now owns attachment slots for optic, barrel, muzzle, underbarrel, magazine, and stock.
- Attachment builds produce an effective weapon definition for ADS, reload, spread, recoil, range, view kick, magazine size, and mobility.
- Account/profile stats now feed the Account tab with K/D, win rate, best weapon, and best operator values.
- Menus/debug HUD now route their primitive output through a game-owned `UiCanvas` command layer that mirrors the planned NanoVG-style API before the final Vulkan text/vector backend lands.
- `UiCanvas` rounded-rectangle commands now emit segmented fallback primitives so menu shell, tabs, and selectable rows can move toward the intended modern visual language before the final vector backend.
- `UiCanvas` now has reusable UI backbone primitives for text metrics, fit-to-width text scaling, shadow/outline text, reusable panels, menu buttons, pills, and dividers.
- SDL debug UI remains available only through explicit legacy launch flags.
- Debug UI now has Gameplay, Network, and Assets pages, toggled with Tab or controller Start/Menu.
- Assets debug UI reports renderer backend plus NovaCore's Vulkan runtime/device summary.
- Assets debug UI reports renderer mesh-resource CPU/GPU residency, pending uploads, failed uploads, deferred destroys, and indexed primitive/vertex/index totals.
- Assets debug UI reports NovaCore backend frame stats: swapchain size/readiness, submitted and skipped frames, swapchain recreate count, and last world box/mesh/line submission counts.
- Assets debug UI reports NovaCore Vulkan UI primitive counts so menu/HUD visibility can be diagnosed from inside the game.
- Plain `nemisis_game` now starts in the Vulkan Main Menu with real 3D/Vulkan UI primitive rendering.
- `nemisis_game --dev-range` starts directly in the Vulkan Dev Range with real 3D GLB mesh rendering.
- `nemisis_game --smoke-test` uses the default Vulkan Main Menu profile for short validation.
- `nemisis_game --menu-flow-smoke-test` now exercises Main Menu, Loadout, Settings, Account, Loading, Dev Range, Team Deathmatch, and Control screens inside the real game executable.
- `nemisis_game --sdl-debug` is the explicit legacy path for old SDL debug UI testing.
- The Dev Range now submits imported GLB meshes to NovaCore's Vulkan world mesh path.
- A1 prototype weapon/operator assets are cataloged, imported, registered, and rendered in the Dev Range path.
- The first-person weapon is now selected from the active loadout and follows the camera/view instead of floating as an independent debug mesh.
- Dev GLB assets are registered once as renderer-owned mesh resources and submitted by stable `MeshResourceHandle`.
- `DevRangeRenderSceneBuilder` now owns Dev Range camera/world/first-person/aim-marker render composition outside `GameApp`.
- Dev Range render tuning loads lighting, FOV, clip planes, and world debug line visibility from `configs/render/dev_range_render.json`.
- Dev Range Vulkan frames now submit world debug lines for aim rays and KCC ground normals through NovaCore.
- Prototype-pack GLBs generated by Blender are cataloged, validated, imported, and instanced in the Dev Range.
- A2 visual test assets generated by Blender are cataloged, validated, imported, registered, preview-rendered, and instanced in the Dev Range.
- The active first-person loadout now prefers imported Project weapon models with A2 fallbacks for missing meshes.
- The dev range requests relative mouse mode through NovaCore while menus keep normal cursor behavior.
- The Dev Shooting Range now owns a deterministic `GreyboxWorld` with floor, walls, cover, ramps, spawns, range markers, and a target lane.
- The Dev Shooting Range now applies greybox collision resolution for bounds, blocking cover/walls, floor grounding, walkable ramps, low steps, ledge blocking, and first mantle candidates.
- Greybox collision is now backed by NovaCore's `novacore::physics::PhysicsWorld` foundation instead of a game-only collision path.
- Dev Range player collision now uses NovaCore swept character movement for fixed-tick displacement before final ground/ramp/wall/mantle resolution.
- Greybox collision now carries explicit contact lists for ground, step, wall, bounds, and sweep contacts, and surfaces those roles in HUD/debug logs/world debug lines.
- The Dev Shooting Range now includes wall-run panels, wall-run surface classification, wall normal/tangent debug data, and first wall-run movement transitions.
- The SDL debug UI draws the greybox as a top-down range map with player position, view direction, target marker, and range helpers.
- Normal dev builds now use NovaCore's SDL3 FetchContent fallback when no installed SDL3 package is present.
- MinGW runtime DLLs are copied beside all Nemisis runtime/test executables for direct shell launches.
- Local player spawning includes a health component for future authoritative damage and respawn flow.
- Weapon metrics estimate damage band, shots-to-eliminate, and measured TTK for 150 HP balance targets.
- `docs/19_PROJECT_KANBAN.md` tracks completed, doing, next, and blocked work until GitHub Projects access is available.
- Movement replay tests cover sprint distance, jump/double-jump, dash cooldown, and config-driven tuning.
- Movement replay tests cover acceleration, friction, sprint convergence, jump/double-jump, dash cooldown, slide duration, slide-jump momentum, and config-driven tuning.
- Input command, weapon simulation, weapon shot, UI canvas, player camera rig, player view, debug target, dev sandbox, player spawn, command queue, command message, asset binding, render tuning, greybox world/collision, and loopback bridge tests cover the newest gameplay bridge.

## Added In Latest Block

- Integrated NovaCore's first swept character movement into the Dev Range player controller path so fixed-tick movement no longer resolves only at the final position.
- Extended `GreyboxCollisionQuery` and `GreyboxCollisionResult` with previous position, sweep enable, max iterations, sweep hit id/kind/normal/fraction, requested/applied/remaining displacement, and iteration count.
- Updated `GameApp` to pass previous player position into the KCC sweep, keep mantle snap as an explicit non-swept resolve, and use sweep normals when cancelling blocked velocity.
- Added in-world Vulkan debug lines for requested KCC movement, applied KCC movement, and sweep hit normals.
- Added Gameplay debug HUD and sandbox-log sweep diagnostics for sweep hit id, hit fraction, and iteration count.
- Added NovaCore and Nemisis regression coverage for high-speed ledge tunneling prevention, low-step traversal through sweeps, wallrun-panel sweep contact, sweep debug lines, and sandbox summary output.
- Verified NovaCore Debug build + CTest, Nemisis Debug build + 32/32 CTest, and direct `nemisis_game.exe --vulkan-dev-range-smoke-test`.

## Previous KCC Stability Block

- Stabilized the Dev Range KCC bridge for the current jump/mantle/wall-run bugs: rising jump arcs now disable ground snap, airborne movement can disable step-up, off-support ledges/steps transition back to Airborne, and wall-run probes use movement tuning.
- Added movement-side coyote time, jump buffering, configurable wall-run minimum height/probe distance, and a real mantle timer so `Mantling` cannot become an infinite gravity-disabled state.
- Tightened GameApp collision handoff so collision grounding, coyote reset, wall-run candidates, mantle activation, and Airborne transitions are owned in one predictable path.
- Extended Gameplay debug telemetry and sandbox logs with coyote, jump-buffer, mantle, and wall-run timers for faster in-game movement diagnosis.
- Upgraded the UI canvas rounded-rectangle command from a no-op rectangle alias to a simple segmented fallback and applied it to the main menu shell, tabs, and selectable rows.
- Removed the accidental untracked Node project artifacts (`node_modules`, `package.json`, and `package-lock.json`) from the Nemisis checkout.
- Added movement and greybox regression tests for rising jump snap prevention, airborne step-up disabling, off-support falling, coyote jump, jump buffer, mantle exit, and wall-run probe tuning.
- Verified NovaCore Debug build + CTest, Nemisis Debug build + 32/32 CTest, and direct `nemisis_game.exe --vulkan-dev-range-smoke-test` on Vulkan 1.4.350 / RTX 3070 Ti with 28/28 dev meshes resident.

## Previous Asset Visibility Block

- Fixed the in-game invisible/incorrectly collapsed asset issue at the game layer by consuming NovaCore's node-transform-aware GLB mesh extraction.
- Retuned the Dev Range A2 asset showcase into an obvious spawn-facing review stage with a backboard, plinths, brighter first-person weapon tints, and A2 operator/weapon/map/hero props placed directly in the default Vulkan view.
- Kept the first-person A2 weapon path active while making the static A2 weapon rack visible enough for immediate visual checks.
- Added `testDevRangeRenderScenePlacesA2AssetsInSpawnView` to verify A2 operator, carbine, rifle, sidearm, and hero prop submissions land in the spawn-facing view volume.
- Verified direct Vulkan window captures after the fix: the A2 stage meshes are visible in the Dev Range and the Vulkan bitmap text orientation is corrected at the engine glyph path.
- Verified `cmake --build --preset windows-msvc-debug --config Debug`, `ctest --test-dir build/windows-msvc-debug -C Debug --output-on-failure`, and direct `nemisis_game.exe --vulkan-dev-range-smoke-test`.

## Previous Mantle Block

- Added NovaCore `PhysicsWorld::probeMantle`, `MantleProbe`, and `MantleProbeResult` for deterministic cover/ledge top detection with obstacle point, target foot position, approach normal, height, distance, surface kind, and collider id.
- Updated NovaCore ledge top resolution so successful mantle target snaps ground cleanly instead of side-blocking the player.
- Added Nemisis `GreyboxCollisionResult` mantle telemetry and wired `GameApp` to query mantle candidates from the current view direction.
- Added `MovementSystem::applyMantleCandidate` and `mantle-climb` movement-tech cue so pressing mantle on a valid candidate moves the KCC to the ledge top and records animation/VFX data.
- Added Dev Range debug lines for mantle candidate paths, Gameplay HUD KCC state reporting, sandbox telemetry for mantle id/height, and placeholder mantle-climb visual boxes.
- Expanded NovaCore smoke, Nemisis greybox collision, movement replay, dev sandbox, and render-scene tests for mantle probe/target/climb/debug coverage.
- Verified NovaCore Debug build + CTest, Nemisis Debug build + 32/32 CTest, and direct `nemisis_game.exe --vulkan-dev-range-smoke-test`.

## Previous Movement Lore/A2 UI Block

- Fixed the Vulkan Main Menu grey-screen path by adding NovaCore Vulkan UI rect/line/text primitive submission for the existing `UiCanvas` debug/menu commands.
- Added backend frame stats for Vulkan UI rects, lines, and text commands and surfaced them in the Nemisis Assets debug page.
- Added `MovementTechState` and `MovementTechCue` so wall-run gravity inverters, double-jump energy platforms, wall-jump detach, and mantle reach are represented as deterministic gameplay-adjacent presentation cues.
- Wired wall-run entry, wall-run continuation/exit, wall jump, double jump, and mantle input into movement-tech cue emission.
- Added first Dev Range placeholder visuals for gravity boot glow, right-arm wall-run activation, double-jump energy platforms, and mantle reach.
- Expanded movement replay and dev sandbox tests to assert the new tech cues.
- Updated the A2 weapon generator toward grounded, realistic carbine/rifle/sidearm blockouts with muted materials and firearm-like detail language while staying original and unbranded.
- Documented the Nemisis movement lore for gravity-inverter boots, left-hand energy-platform double jumps, and future mantle/climb animation requirements.
- Regenerated the A2 visual pack and preview with Blender 5.1.2; the current dev asset import reports 489 primitives, 36,224 vertices, and 56,112 indices across the required dev renderables.
- Verified `cmake --build --preset windows-msvc-debug --config Debug`, full `ctest` with 32/32 passing tests, `nemisis_game.exe --smoke-test`, and `nemisis_game.exe --vulkan-dev-range-smoke-test`.
- Verified direct Vulkan runtime logs show UI primitive submission in both default Main Menu and Dev Range smoke paths.

## Previous A2/PhysicsWorld Block

- Added NovaCore-backed `PhysicsWorld` integration to Nemisis greybox collision, including surface-kind mapping for floor, wall, ramp, cover, slide ramp, ledge, wall-run panel, and trigger primitives.
- Added four wall-run panel primitives to the Dev Shooting Range and surfaced wall-run contact, wall normal, wall tangent, and wall primitive ids in debug telemetry/UI.
- Added first wall-run movement behavior: contact entry, tangent velocity carry, timer-based exit, double-jump preservation, and wall-jump impulse away from the wall.
- Generated the A2 visual FPS test pack with Blender 5.1.2: blackout-style carbine, modular rifle, striker sidearm, pilot/operator proxy, wall-run panel, slide ramp, cover crate, and range hero prop.
- Cataloged the A2 `.blend`, `.glb`, metadata, and manifest files as required dev renderables, raising the required visual asset set to 28 ids.
- Added `tools/blender/render_a2_preview.py` and rendered `assets/generated/a2_visual_pack/a2_visual_pack_preview.png` for asset review.
- Updated the Dev Range render scene so A2 assets are instanced in-world and the first-person weapon uses the A2 loadout models.
- Expanded collision, movement replay, dev sandbox, asset binding, and render-scene tests for the new PhysicsWorld bridge, wall-run surface behavior, and A2 renderables.
- Verified NovaCore Debug build plus smoke tests and Nemisis Debug build plus 32/32 CTest suite.
- Verified direct `nemisis_game.exe --vulkan-dev-range-smoke-test`: Vulkan 1.4.350 on `NVIDIA GeForce RTX 3070 Ti`, 30 catalog assets, 28/28 required dev meshes, 460 imported primitives, 35,528 vertices, 55,068 indices, 33 world boxes, 27 mesh instances, and active world-line debug submission.

## Previous Menu/Backend Stats Block

- Added an in-process `nemisis_game --menu-flow-smoke-test` path that validates the menu stack through the normal game executable instead of a separate runner.
- Added deterministic menu-flow stages for Play, Loadout, Settings, Account, Dev Range loading, live Dev Range, TDM loading, TDM placeholder, Control loading, and Control placeholder.
- Exposed NovaCore backend frame stats to the Nemisis UI path and surfaced swapchain readiness, swapchain size, submitted/skipped frames, recreate count, and current frame draw submission counts on the Assets debug page.
- Updated `GameMenu` with explicit screen-driving helpers so tests, future automation, and gameplay flow can route through the same Main Menu/Loading/Gamemode surface.
- Added CTest coverage through `nemisis_game_menu_flow_smoke`, raising the suite to 32 passing tests.
- Verified Debug build, `ctest --test-dir build/windows-msvc-debug -C Debug`, `nemisis_game.exe --menu-flow-smoke-test`, and `nemisis_game.exe --vulkan-dev-range-smoke-test`.

## Previous Multi-Target Block

- Added `DevTargetRange` as the first multi-target range model with four lanes, active lane tracking, nearest-hit selection, per-lane respawn timers, alive/down counts, and full reset support.
- Split `DebugTarget` tracing from damage application so ray/sphere hit tests can select the correct lane before mutating target health.
- Replaced the old single target runtime path in `GameApp` with range-wide hit resolution while keeping an active target snapshot for HUD compatibility.
- Updated Vulkan Dev Range rendering to draw lane target meshes, stands, and target hit boxes from `DevTargetRangeState`; current smoke submits 29 world boxes and 19 mesh instances.
- Updated the HUD/topdown map/debug telemetry to show multi-lane target state, active lane name, alive/down counts, and lane-aware event text.
- Added `nemisis_dev_target_range_tests` and expanded debug target/render scene/sandbox tests, raising the suite to 31 passing tests.
- Verified Debug build, `ctest --test-dir build/windows-msvc-debug -C Debug`, `nemisis_game.exe --smoke-test`, and `nemisis_game.exe --vulkan-dev-range-smoke-test`.

## Previous Block

- Added `DevRangeSession` as a focused gameplay-session layer for the playable range: shots fired, hits, eliminations, accuracy, current/best streak, damage dealt, range reset count, target respawn, player respawn, regen delay, and short event text.
- Persisted player-facing runtime settings and loadout selections through `configs/user/nemisis_user.json` using `UserSettingsPersistence`, while preserving config-driven defaults when no user file exists.
- Added `ResetRange` input on keyboard `P` to reset the Dev Range, player transform, camera rig, health, command queue, weapon runtime, and target state. Controller `Y` now belongs to weapon pickup.
- Wired `PlayerHealthComponent` into the Dev Range sample and HUD path so health, down/respawn state, and future damage/regen validation have a single component-owned source.
- Expanded the Dev Range HUD with health, ammo, range score, accuracy, streak, best streak, target respawn timer, and session event text.
- Updated debug telemetry so the Gameplay page reports player HP and range score/accuracy alongside KCC state.
- Added focused unit tests for user settings persistence and Dev Range session behavior, raising the local suite to 30 passing tests.
- Verified `cmake --build --preset windows-msvc-debug --config Debug`, `ctest --test-dir build/windows-msvc-debug -C Debug`, `nemisis_game.exe --smoke-test`, and `nemisis_game.exe --vulkan-dev-range-smoke-test`.

## Earlier UI/A1 Block

- Added A1 asset production through the Blender/Codex asset-agent path: compact rifle, modern rifle, compact sidearm, stylized operator, and first-person arms.
- Registered the A1 assets as required Dev Sandbox renderables and verified 20/20 required renderables import, extract CPU mesh data, register with NovaCore, and become Vulkan resident in smoke runs.
- Changed the default launch profile so plain `nemisis_game` starts in the Vulkan Main Menu, while `--dev-range` remains the explicit direct Firing Range path.
- Expanded `GameMenu` into a deeper prototype shell with Play, Gamemodes, Loadout, Character, Settings, Account, and Loading screens.
- Added `UiCanvas::image` placeholder commands plus SVG-backed UI art for Firing Range, Loadout, Operator, Settings, and Dev Range loading.
- Added runtime `GameSettings` for mouse/controller sensitivity, ADS look scaling, aim assist toggles, damage number visibility, HUD scale, safe area, and debug world-line visibility.
- Added `WeaponAttachments` with six slots, prototype attachments, loadout cycling, and effective-weapon build summaries.
- Wired the active attachment build into `GameApp` so weapon simulation, shot tracing, ammo caps, loading UI, and HUD all read the effective weapon.
- Added `PlayerProfile` prototype stats for Account UI: K/D, win rate, best weapon, best operator, damage per match, and accuracy.
- Added Q/E and controller shoulder tab navigation, with Left/Right kept for live Loadout/Settings adjustment.
- Reworked first-person Dev Range weapon rendering so the active weapon asset follows the camera forward/right vectors, ADS blend, weapon sway, and recoil offsets; added view-held hands and muzzle feedback boxes.
- Updated Dev Range static showcase to include the A1 operator and modern rifle assets.
- Added `nemisis_weapon_attachments_tests`, `nemisis_game_settings_tests`, and `nemisis_player_profile_tests`, raising the suite to 28 passing tests.
- Verified `cmake --preset windows-msvc-debug`, full Debug build, direct `nemisis_game.exe --smoke-test`, and `ctest -C Debug` with 28/28 passing tests.

## Earlier Movement/UI Block

- Added acceleration/friction-based ground movement, air steering limits, air drag, slide timers, slide steering, slide end speed, slide jump boost, dash duration, and horizontal speed telemetry.
- Added hot-reload movement config fields for ground, air, slide, and dash feel tuning.
- Added `PlayerCameraRig` as the first natural FPS camera layer with smoothed eye position, recoil view offsets, FOV kick, head bob, roll, weapon sway, speed fraction, and ADS fraction.
- Wired the camera rig into `GameApp` and `DevRangeRenderSceneBuilder` so the Vulkan Dev Range uses a smoothed visual FPS camera and weapon placement offsets.
- Expanded weapon definitions with recoil recovery, max recoil clamps, recoil noise, ADS recoil scale, movement/air/sprint spread penalties, ADS FOV multiplier, and view-kick tuning.
- Expanded weapon runtime simulation with ADS blend, burst shot count, recoil accumulation/recovery, reload progress, movement spread telemetry, and deterministic visual kick.
- Expanded shot tracing so ADS alpha, runtime recoil, sprinting, airborne state, and movement speed shape deterministic hitscan directions.
- Added `UiCanvas` as the game-owned NanoVG-style command layer and routed `GameMenu` debug primitives through it.
- Added `nemisis_ui_canvas_tests` and `nemisis_player_camera_rig_tests`, raising the local suite to 25 passing tests.
- Verified `cmake --preset windows-msvc-debug`, full Debug build, Vulkan shader compilation through `F:\VulkanSDK\1.4.350.0`, and `ctest -C Debug` with 25/25 passing tests.

## Earlier Blocks

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
- Fixed the direct `cmake-build-debug/nemisis_game.exe` no-window path by enabling automatic SDL3 fetch/build in NovaCore.
- Fixed direct MinGW executable launch by copying runtime DLLs beside `nemisis_game.exe` and test executables.
- Cleaned initial config watching so failed startup config loads are logged instead of ignored.
- Added `GreyboxWorld` as the first data-owned dev range map with stable primitive ids and world-space spawn data.
- Added a visible top-down debug range inside the Dev Shooting Range so player/world/target layout can be inspected before full 3D mesh rendering.
- Local player spawn now comes from the greybox world definition.
- Added `nemisis_greybox_world_tests` and raised the local test suite to 17 passing tests in the current debug build.
- Expanded Blender automation to generate target dummy, player capsule proxy, first-person arms, soldier proxy, AR, SMG, sidearm, and arena-kit A0 assets.
- Generated the first `.blend`, `.glb`, and metadata files under `assets/source/blender` and `assets/export/gltf` with Blender 5.1.
- Added `DevAssetBindings` to bind the generated A0 glTF metadata into NovaCore mesh handles at startup.
- Dev sandbox startup reports required A0 mesh readiness before entering the playable sandbox.
- Added greybox collision queries and applied them after player movement simulation.
- Debug telemetry now reports collision hit/block state, and the Assets debug page reports mesh/metadata readiness.
- Added `nemisis_dev_asset_bindings_tests` and `nemisis_greybox_collision_tests`, raising the local test suite to 19 passing tests.
- Added NovaCore-backed GLB scene-info imports for the generated A0 assets.
- Dev sandbox startup now reports imported scene totals, currently `Dev mesh assets ready: 8/8 metadata=8 imported=8 meshes=152 nodes=215 materials=34`.
- Assets debug page now reports imported asset count plus total imported mesh/node counts.
- `nemisis_dev_asset_bindings_tests` now writes tiny GLB fixtures and verifies imported scene info is stored on `MeshCatalog` entries.
- Added NovaCore-backed CPU mesh extraction for generated A0 GLBs.
- Dev sandbox startup now reports extracted totals, currently `primitives=152 vertices=21368 indices=32304`.
- Assets debug page now reports extracted asset count, primitive count, vertex count, and index count.
- `nemisis_dev_asset_bindings_tests` now verifies extracted GLB mesh data is stored on `MeshCatalog` entries.
- NovaCore Vulkan runtime probing is surfaced in Nemisis startup logs and the Assets debug page; current local smoke runs detect Vulkan 1.4.350 on `NVIDIA GeForce RTX 3070 Ti (discrete)`.
- Dev Shooting Range now overlays a budgeted A0 environment GLB wireframe from extracted CPU mesh positions/indices on the debug range map.
- Added game launch options for `--vulkan` and `--vulkan-smoke-test`.
- Added `nemisis_game_vulkan_smoke`, raising the local test suite to 20 passing tests.
- Verified the compiled NovaCore Vulkan backend with SDK `F:\VulkanSDK\1.4.350.0`: startup logs now show Vulkan 1.4.350, swapchain creation, world box graphics pipeline creation, and active device `NVIDIA GeForce RTX 3070 Ti`.
- Added Dev Range autostart with `--dev-range` and `--vulkan-dev-range-smoke-test`.
- The Dev Range now feeds a 3D camera plus greybox primitive boxes into NovaCore's Vulkan world renderer.
- Added `nemisis_game_vulkan_dev_range_smoke`, raising the local test suite to 21 passing tests.
- Added a NovaCore Vulkan world mesh path with device-local vertex/index buffers, staging uploads, and indexed draw submission.
- Added `RenderMesh3D` submissions from Nemisis for the A0 arena, target/soldier, weapon, and generated prototype-pack assets.
- Added `tools/blender/make_prototype_pack.py` plus seven generated GLBs for SMG, humanoid, wall, floor, crate, ramp, and target stand.
- Added prototype-pack entries and metadata to `configs/assets/nemisis_assets.json`.
- Dev sandbox startup now reports `Dev mesh assets ready: 15/15 metadata=15 imported=15 meshes=228 nodes=303 materials=58 primitives=228 vertices=24470 indices=37224`.
- Verified Vulkan Dev Range smoke uploads 11 mesh assets and submits 12 mesh instances alongside 16 greybox boxes.
- Made Vulkan 3D Dev Range the default launch profile with `require_vulkan=true`, `start_screen=dev_range`, and `lock_dev_range=true`.
- Added a first-person arms proxy and 3D aim marker to the Vulkan Dev Range frame, raising the current smoke submission to 13 mesh instances and 21 world boxes.
- Promoted Dev Range GLB rendering from transient per-frame CPU mesh pointers to renderer-owned mesh-resource registration.
- Added game-side dev mesh resource registration/release and startup logging for 15/15 registered resources.
- Added `DevRangeRenderSceneBuilder` with scene submission stats for world boxes, mesh instances, skipped meshes, first-person meshes, and aim-marker boxes.
- Added `nemisis_dev_range_render_scene_tests`, raising the local test suite to 22 passing tests.
- Verified Vulkan Dev Range smoke now reports 15 resident mesh resources and submits 13 mesh instances alongside 21 world boxes.
- Added configurable Dev Range render tuning under `configs/render/dev_range_render.json`.
- Added `nemisis_render_tuning_tests`, raising the local test suite to 23 passing tests.
- Added walkable ramp sampling, step-up handling, ground normals, ground surface ids, and ledge blocking to greybox collision.
- Added low-step and mid-ledge training pieces to the Dev Range greybox.
- Added world-space Vulkan debug lines for aim rays and KCC ground normals.
- Verified Vulkan Dev Range smoke now creates the world line pipeline and submits 23 world boxes, 13 mesh instances, and at least one world debug line.

## Next Game Blocks

- Feed player damage and respawn through authoritative hit events and future server validation.
- Add timed range drills, measured TTK panels, and recoil-control scoring on top of the multi-lane target range.
- Expand debug UI pages with frame timings, entity counts, packet loss simulation, and reconciliation error.
- Expand greybox collision into mantle probes, slide validation, and richer slope/step debug visualization.
- Add resize-safe Vulkan swapchain recreation and validation/debug labels.
- Add richer material fallback controls and resize-safe Vulkan swapchain recreation.
