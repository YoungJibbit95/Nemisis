# 19 Project Kanban

This file mirrors the active work locally while GitHub Projects also track the larger repo-level milestones.

## Done

- [x] Split NovaCore engine and Nemisis game repositories.
- [x] Add C++23 CMake skeletons.
- [x] Add fixed-tick runtime loop.
- [x] Add custom ECS foundation.
- [x] Add window/input snapshot bridge.
- [x] Add MKB and controller gameplay input.
- [x] Add transient mouse look axes.
- [x] Add movement command simulation foundation.
- [x] Add AR/SMG/shotgun/sidearm weapon data and simulation.
- [x] Add local player entity composition.
- [x] Add command queue and loopback command acknowledgement.
- [x] Add first debug target, shot traces, and target damage.
- [x] Add NovaCore asset manifest, registry, and streaming queue backbone.
- [x] Add Nemisis game asset catalog.
- [x] Add Blender asset plan, agent briefs, and generator script.
- [x] Fix `windows-msvc-debug` so it no longer depends on Ninja or an unset `VCPKG_ROOT`.
- [x] Add SDL debug renderer primitives and bitmap debug text.
- [x] Add barebones main menu, mode selection, dev shooting range entry, and debug overlay.
- [x] Add NovaCore packet bitstream usage for Nemisis command/ack payloads.
- [x] Add relative mouse request flow for active dev-range gameplay.
- [x] Add player health/damage helpers and measured TTK tests.
- [x] Expand debug UI into Gameplay, Network, and Assets pages.
- [x] Fix visible dev launches by auto-fetching SDL3 when vcpkg/package SDL3 is missing.
- [x] Copy MinGW runtime DLLs beside executable targets for direct shell runs.
- [x] Add NovaCore SDL debug line primitives for world/map overlays.
- [x] Add Nemisis `GreyboxWorld` data for the first Dev Shooting Range.
- [x] Draw a visible top-down greybox range with player, view direction, target, cover, ramps, and range markers.
- [x] Spawn the local player from the greybox world definition.
- [x] Generate A0 Blender source/export assets for target, player proxy, first-person arms, soldier proxy, AR, SMG, sidearm, and arena kit.
- [x] Add NovaCore glTF metadata validation and mesh-handle placeholders.
- [x] Bind generated A0 Nemisis assets into NovaCore `MeshCatalog` handles.
- [x] Add first greybox collision queries for bounds, cover/walls, and floor grounding.
- [x] Surface mesh/metadata readiness and collision state in debug telemetry/UI.
- [x] Add NovaCore GLB scene-info import and attach imported A0 scene totals to Nemisis mesh handles.
- [x] Surface imported asset and mesh/node totals in startup logs and the Assets debug page.
- [x] Add NovaCore CPU GLB mesh extraction for positions, optional normals/UVs, and indices.
- [x] Attach extracted A0 mesh data to Nemisis mesh handles and surface primitive/vertex/index totals.
- [x] Add NovaCore Vulkan runtime/device probe and surface detected GPU/runtime in logs/debug UI.
- [x] Add A0 environment GLB wireframe preview from extracted CPU mesh data.
- [x] Expose Vulkan SDK `F:\VulkanSDK\1.4.350.0` to the CMake build.
- [x] Add NovaCore compiled Vulkan backend with swapchain, render pass, framebuffers, sync, clear/present, and first graphics pipeline.
- [x] Add Vulkan shader compilation through `glslc`.
- [x] Add Nemisis `--vulkan`/`--vulkan-smoke-test` and CTest coverage.
- [x] Add NovaCore depth-tested Vulkan world box primitive path.
- [x] Feed Nemisis Dev Range greybox primitives into the Vulkan 3D renderer.
- [x] Add `--vulkan-dev-range-smoke-test` and CTest coverage.
- [x] Add NovaCore Vulkan GLB mesh upload with staging buffers and indexed draw submission.
- [x] Submit A0 and prototype-pack GLB meshes from Nemisis Dev Range into the Vulkan 3D renderer.
- [x] Generate Blender prototype-pack GLBs for SMG, humanoid, wall, floor, crate, ramp, and target stand.
- [x] Catalog and validate prototype-pack assets as required Dev Sandbox renderables.
- [x] Make `nemisis_game --dev-range` boot directly into the Vulkan 3D Dev Range.
- [x] Disable silent SDL debug fallback for the default Vulkan launch profile.
- [x] Add explicit legacy `--sdl-debug` launch flags.
- [x] Add first-person arms proxy and 3D aim marker to the Vulkan Dev Range.
- [x] Promote mesh rendering into renderer-owned resource handles, Vulkan upload queue, residency stats, and deferred destruction.
- [x] Add `DevRangeRenderSceneBuilder` and `nemisis_dev_range_render_scene_tests`.
- [x] Add NovaCore Vulkan world debug line pipeline and submit Dev Range aim/ground-normal lines.
- [x] Add configurable Dev Range render tuning and `nemisis_render_tuning_tests`.
- [x] Add KCC ramp height sampling, low-step handling, ground normals, and ledge blocking tests.
- [x] Replace snap movement with acceleration, friction, air control, slide duration, slide steering, dash duration, and slide-jump momentum.
- [x] Add `PlayerCameraRig` for natural FPS camera smoothing, FOV kick, head bob, recoil view offsets, weapon sway, and ADS fraction.
- [x] Expand weapon runtime with ADS blend, recoil accumulation/recovery, burst tracking, reload progress, movement spread, and view-kick telemetry.
- [x] Expand hitscan traces with runtime recoil, ADS alpha, airborne/sprint spread penalties, and movement speed.
- [x] Add game-owned `UiCanvas` command layer and route menu/debug primitives through it as the NanoVG-style bridge.
- [x] Add `nemisis_ui_canvas_tests` and `nemisis_player_camera_rig_tests`.
- [x] Verify Windows MSVC Vulkan build and 25/25 CTest suite.
- [x] Generate A1 Blender assets for compact rifle, modern rifle, compact sidearm, stylized operator, and first-person arms.
- [x] Catalog, import, extract, register, and Vulkan-reside 20/20 required Dev Sandbox renderables.
- [x] Make plain `nemisis_game` boot into the Vulkan Main Menu while keeping `--dev-range` for direct range launch.
- [x] Expand main menu into Play, Gamemodes, Loadout, Character, Settings, Account, and Loading screens.
- [x] Add live mouse/controller settings, ADS look scaling, HUD scale, damage-number toggles, and aim-assist flags.
- [x] Add six-slot weapon attachment loadouts and effective-weapon build summaries.
- [x] Feed active attachment builds into weapon simulation, shot tracing, HUD, and loading UI.
- [x] Add Account/Profile stats for K/D, win rate, best weapon, and best operator.
- [x] Bind active first-person weapon and arms to camera/view, ADS, sway, and recoil.
- [x] Add `nemisis_weapon_attachments_tests`, `nemisis_game_settings_tests`, and `nemisis_player_profile_tests`.
- [x] Verify Windows MSVC Vulkan build, direct smoke run, and 28/28 CTest suite.
- [x] Persist runtime settings and active loadout to `configs/user/nemisis_user.json`.
- [x] Add Dev Range session scoring, accuracy, streaks, target respawn, reset feedback, and player health HUD data.
- [x] Add keyboard `P` and controller `Y` Dev Range reset action.
- [x] Add `nemisis_user_settings_persistence_tests` and `nemisis_dev_range_session_tests`.
- [x] Verify Windows MSVC Vulkan build, direct smoke runs, and 30/30 CTest suite.
- [x] Add four-lane `DevTargetRange` with nearest-hit selection, active lane tracking, per-lane respawn, and full reset.
- [x] Render multi-lane Dev Range targets and stands through the Vulkan mesh path.
- [x] Surface target alive/down counts and active lane state in HUD, topdown map, debug telemetry, and sandbox logs.
- [x] Add `nemisis_dev_target_range_tests` and expand target/render/sandbox coverage.
- [x] Verify Windows MSVC Vulkan build, direct smoke runs, and 31/31 CTest suite.
- [x] Add `nemisis_game --menu-flow-smoke-test` through the real game executable for Main Menu, Loading, Dev Range, TDM, and Control screen coverage.
- [x] Surface NovaCore Vulkan backend frame stats in the Nemisis Assets debug page.
- [x] Verify Windows MSVC Vulkan build, menu-flow smoke, Dev Range Vulkan smoke, and 32/32 CTest suite.
- [x] Add NovaCore `PhysicsWorld` bridge for Nemisis greybox collision.
- [x] Add wall-run panel greybox primitives, wall-run surface metadata, wall normals, and wall tangents.
- [x] Add first wall-run movement transition and wall-jump replay coverage.
- [x] Generate A2 Blender visual pack for blackout carbine, modular rifle, striker sidearm, pilot/operator, wall-run panel, slide ramp, cover crate, and range hero prop.
- [x] Catalog, import, extract, register, and render 28/28 required Dev Sandbox renderables.
- [x] Add A2 asset preview rendering through `tools/blender/render_a2_preview.py`.
- [x] Verify NovaCore smoke tests and Nemisis 32/32 CTest suite after the A2/PhysicsWorld block.
- [x] Add Vulkan-native UI primitive rendering for `UiCanvas` rects, lines, and bitmap debug text.
- [x] Surface Vulkan UI draw counts in backend stats and the Nemisis Assets debug page.
- [x] Add movement-tech cues for wall-run gravity activation, gravity boots, double-jump energy platforms, wall-jump detach, and mantle reach.
- [x] Add Dev Range placeholder visuals for gravity boots, arm-button activation, energy-step platforms, and mantle reach.
- [x] Shift A2 weapon generator output toward grounded, realistic, unbranded carbine/rifle/sidearm blockouts.
- [x] Document movement lore and animation requirements for gravity-inverter boots, energy-platform double jumps, and future mantle/climb.
- [x] Add NovaCore mantle probes for reachable cover/ledge tops and ledge-top standing resolution.
- [x] Wire Nemisis mantle candidates through GreyboxCollision, GameApp, MovementSystem, HUD/debug telemetry, and Dev Range debug lines.
- [x] Add `mantle-climb` movement-tech cue and replay/debug/render tests for the first mantle foundation.
- [x] Add NovaCore KCC snap controls for rising jump arcs, airborne step-up gating, configurable wall-run probes, and off-support ledge falling.
- [x] Add Nemisis movement coyote time, jump buffering, wall-run probe tuning, and timed mantle exit.
- [x] Fix Dev Range collision handoff so jump, mantle, wall-run, grounded, and airborne transitions no longer fight each other.
- [x] Add Gameplay debug telemetry for coyote, jump buffer, mantle timer, and wall-run timer.
- [x] Upgrade `UiCanvas` rounded-rectangle commands and apply the first modernized main-menu shell/tabs/rows.
- [x] Remove accidental untracked Node artifacts from the Nemisis checkout.
- [x] Add NovaCore `PhysicsWorld::sweepCharacter` with first-hit fraction, normal, collider id, applied displacement, remaining displacement, and iteration telemetry.
- [x] Integrate swept KCC movement into Nemisis Dev Range player collision before final ground/ramp/wall/mantle resolution.
- [x] Add Vulkan world debug lines and HUD/log diagnostics for requested/applied sweep movement and sweep hit normals.
- [x] Fix Vulkan UI/text screen-space orientation and glyph bit order through NovaCore shaders/backend.
- [x] Add NovaCore character contact manifolds for ground, step, wall, bounds, and sweep contacts.
- [x] Surface contact roles through Nemisis GreyboxCollision, Dev Sandbox logs, Gameplay debug HUD, and Dev Range world debug lines.
- [x] Expand `UiCanvas` with text metrics, fit scaling, shadow/outline text, panels, buttons, pills, and dividers.
- [x] Convert mantle from instant snap to a deterministic fixed-tick climb curve with grounded completion handoff.
- [x] Add NovaCore short-tap input edge retention and pointer-position support.
- [x] Add held input fields for jump, slide, mantle, and reload in gameplay commands plus command packet protocol v2.
- [x] Add slide buffering and held-slide latching for reliable sprint-slide activation.
- [x] Fix mantle activation so held jump/mantle can catch valid ledges and active climb interpolation is not overwritten by collision resolve.
- [x] Improve NovaCore KCC ramp/slide grounding for continuous ramp traversal while preserving disabled airborne step-up.
- [x] Add clickable GameMenu tabs and selectable rows through the unified `nemisis_game` UI path.
- [x] Add temporary A2 runtime collision proxies for visible asset-stage ramp, wall-run panel, cover crate, hero prop, plinths, and backboard.
- [x] Add sibling-folder Project GLB runtime bridge and Dev Range preview placement for user-provided character/weapon assets.
- [x] Add asset-readiness audit tooling/report for Blender/GLB import validation.
- [x] Move user-provided Project character/weapon GLBs into `assets/project_assets` and switch catalog/metadata to repo-local paths.
- [x] Copy and catalog `skybox1.glb` as a first-pass Dev Range sky environment mesh.
- [x] Add NovaCore pitch/roll support for `RenderMesh3D` and use it for camera-following first-person weapon/arms placement.
- [x] Remove first-person debug weapon/hand boxes and scale the real imported weapon models for visible ADS testing.
- [x] Lower the local player body mesh out of the camera head while keeping the Project character visible for player/dummy representation.
- [x] Add Dev Range weapon pickup pads, pickup input, direct weapon switching, and command protocol v3 coverage.
- [x] Improve wall-run entry gates, wider probes, double-jump ordering, wall-run camera lean, and ADS FOV feedback.
- [x] Verify Nemisis 32/32 CTest and NovaCore 1/1 CTest after Project asset integration.

## Doing

- [ ] Expand material fallback controls, validation labels, and renderer resize stress coverage.
- [ ] Keep UI moving from clickable Vulkan primitive path toward SDF/MSDF text, clipping, batching, focus graph, and richer vector drawing.
- [ ] Start texture/material binding for A1/A2 GLBs so imported assets look less like tinted blockouts.
- [ ] Grow the multi-target Dev Range into timed drills, TTK panels, and recoil-control scoring.
- [ ] Deepen KCC physics beyond the current swept AABB/contact-manifold pass into true capsule sweeps, moving platform contacts, slide validation, and server replay validation.
- [ ] Author or generate Project-asset sockets (`socket_muzzle`, `socket_weapon_root`, `socket_camera`, hands) so the runtime can stop using placement heuristics.
- [ ] Replace first-person mesh offsets with authored first-person arm/weapon animation clips once Blender-ready rigs exist.

## Next Core

- [ ] Cursor policy, raw-input options, and richer controller response curves.
- [ ] Packet simulation for latency, jitter, loss, duplication, and reorder.
- [ ] Vulkan validation debug labels and resize stress tests.
- [ ] Renderer texture/material residency and cross-zone reference counting.
- [ ] Timed mantle attach/climb curves, slide validation, richer slope/step debug visualization, and capsule-vs-world contact manifolds.
- [ ] Wall-run camera feel, wall detach rules, wall-run cooldown, and surface eligibility tuning.
- [ ] Convert movement-tech debug visuals into authored first-person and third-person animation clips.
- [ ] Add a dedicated NovaCore skybox/cubemap pass instead of rendering sky as a large regular GLB mesh.
- [ ] Shared server validation for greybox collision and movement corrections.
- [ ] Feed camera/weapon telemetry into prediction-safe replay snapshots for future reconciliation debugging.

## Next Gameplay

- [ ] TDM placeholder match lifecycle.
- [ ] Control placeholder objective lifecycle.
- [ ] Timed range drills, lane score breakdowns, and measured TTK panels.
- [ ] Player damage sources, down-state, respawn flow, and future server validation.
- [ ] Recoil/ADS/range-score HUD widgets fed from runtime telemetry.

## Blocked

- [ ] No current engine-core blocker.
