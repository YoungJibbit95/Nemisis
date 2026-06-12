# 21 Dev Range Session And User State

## Goal

The Dev Shooting Range must be a stable playable loop while the engine backbone is still growing. It should let us test camera feel, weapon tuning, attachment behavior, player HUD, target feedback, and later prediction/reconciliation without waiting for a full match mode.

This document tracks the current session layer and the user-state persistence slice.

## Runtime Ownership

Current ownership:

- `GameApp` orchestrates startup, config load, user settings load, fixed tick simulation, frame UI updates, and runtime persistence.
- `DevRangeSession` owns range-specific scoring and timers.
- `DevTargetRange` owns multi-lane target state, hit selection, active lane state, and lane respawns.
- `PlayerHealthComponent` remains the player health authority for the local sample.
- `DebugTargetState` remains the per-target health and hit-state primitive inside each target lane.
- `GameMenu` renders the current HUD/debug view from a `DevSandboxSample`.
- `UserSettingsPersistence` serializes/deserializes player-facing settings and loadout choices.

The important boundary is that `DevRangeSession` does not own world, renderer, input, weapon state, or the target lane container. It only tracks the session outcome and transient feedback. That keeps it small enough to replace or mirror on a server later.

## Dev Target Range Data

`DevTargetRangeState` currently tracks:

- Four target lanes.
- Active lane index.
- Per-lane display names.
- Per-lane `DebugTargetState`.
- Per-lane respawn timers.

Default lanes:

- `left_15m`
- `center_20m`
- `right_25m`
- `upper_22m`

Hit selection:

- Weapon shot traces are tested against every alive lane.
- The nearest valid ray/sphere hit wins.
- Only the winning lane mutates health.
- The winning lane becomes the active lane for HUD/debug display.
- Eliminated lanes continue counting as down until their lane respawn timer completes.

## Dev Range Session Data

`DevRangeScoreboard` currently tracks:

- `shotsFired`
- `shotsHit`
- `targetsEliminated`
- `rangeResets`
- `playerRespawns`
- `currentStreak`
- `bestStreak`
- `damageDealt`

`DevRangeSessionState` currently tracks:

- Scoreboard data.
- Target respawn countdown.
- Player respawn countdown.
- Player regen delay.
- Event text.
- Event text countdown.

`DevRangeSessionTuning` currently exposes:

- Target respawn delay.
- Player respawn delay.
- Event text display time.
- Regen delay after damage.
- Regen rate.

## Session Flow

Startup:

1. Config defaults load first.
2. User settings/loadout snapshot overlays config defaults if `configs/user/nemisis_user.json` exists.
3. Assets, renderer, ECS entities, weapon registry, and input bindings initialize.
4. The default profile opens the Vulkan Main Menu.
5. `--dev-range` or the Firing Range menu entry enters the playable range.

Fixed tick:

1. Local input builds a command.
2. Movement, camera view, weapon runtime, and command queue update.
3. Hitscan runs when the weapon fires.
4. Target range hit result selects and updates the nearest hit lane.
5. `DevRangeSession` records fired/hit/elimination/damage telemetry.
6. Target lane respawn timers restore individual targets when complete.
7. Player health regen/respawn timers tick from the health component.
8. `DevSandboxSample` collects all telemetry for HUD/debug rendering.

Frame:

1. Menu and HUD consume input actions.
2. Loadout and settings edits are applied live.
3. Changed settings/loadout snapshots are written to user config.
4. `ResetRange` can restore range state while gameplay is active.
5. The Vulkan 3D renderer receives the Dev Range render scene.

## Reset Behavior

Keyboard `P` and controller `Y` reset the current Dev Shooting Range.

Reset currently restores:

- Target health and hit state.
- All target lanes and lane respawn timers.
- Session event feedback.
- Player movement state and spawn position.
- Player transform.
- Camera transform.
- Player view angles.
- Player health.
- Weapon runtime state and ammo.
- Local command queue.
- Camera rig smoothing state.

The reset is intentionally stronger than a simple target reset. It gives us a repeatable test baseline for movement, camera, recoil, attachments, and future network replay snapshots.

## User Settings Snapshot

The user snapshot file is:

```text
configs/user/nemisis_user.json
```

It stores:

- Mouse sensitivity X/Y.
- Mouse ADS multiplier.
- Mouse invert Y.
- Controller look sensitivity X/Y.
- HUD scale.
- Damage-number toggle.
- Aim-assist toggle and prototype values.
- Active weapon id.
- Six-slot attachment loadout.

Load rules:

- Missing file is valid and uses config defaults.
- Invalid JSON reports warnings and falls back to defaults.
- Unknown attachment ids are ignored so old user files do not block startup.
- The game only saves when the serialized snapshot changes.

## HUD And Debug

The Dev Range HUD currently shows:

- Weapon display name.
- Active attachment build.
- Ammo.
- Player health bar.
- Target health.
- Target respawn timer.
- Active lane name.
- Target lanes alive/down count.
- Eliminations.
- Accuracy.
- Current streak.
- Best streak.
- Session event text.
- Reset hint.

The Gameplay debug page currently shows:

- Screen.
- Movement mode.
- Tick.
- Input device.
- Position.
- Collision/KCC state.
- Player HP.
- Eliminations.
- Accuracy.
- Current streak.
- Target lanes alive/down count in sandbox telemetry.

## Test Coverage

Current focused tests:

- `nemisis_user_settings_persistence_tests`
- `nemisis_dev_range_session_tests`
- `nemisis_dev_target_range_tests`
- `nemisis_game_menu_tests`
- `nemisis_dev_sandbox_tests`
- `nemisis_player_health_tests`
- `nemisis_weapon_attachments_tests`
- `nemisis_game_settings_tests`
- `nemisis_weapon_shot_tests`

Latest verified suite:

```powershell
ctest --test-dir build/windows-msvc-debug -C Debug --output-on-failure
```

Result:

```text
31/31 tests passed
```

Latest direct runtime checks:

```powershell
.\build\windows-msvc-debug\Debug\nemisis_game.exe --smoke-test
.\build\windows-msvc-debug\Debug\nemisis_game.exe --vulkan-dev-range-smoke-test
```

Both detect Vulkan 1.4.350 on the local RTX 3070 Ti path and register 20/20 required Dev Sandbox mesh resources. The Dev Range smoke currently submits 29 world boxes, 19 mesh instances, and at least one world debug line.

## Next Upgrades

- Add timed range drills for accuracy, TTK, recoil control, and movement shooting.
- Add per-lane score breakdowns.
- Feed enemy/target damage sources into player health so down-state and respawn can be tested.
- Mirror range session state through server-authored events.
- Add packet loss/jitter overlays beside session score once network simulation is active.
- Add recoil and ADS graphs to the HUD/debug pages.
- Persist richer controller response curves and raw-input/cursor policy settings.
