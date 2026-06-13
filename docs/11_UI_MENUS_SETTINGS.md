# 11 UI Menus Settings

## UI Goal

The game UI should feel modern, smooth, and custom. It should support rounded edges, blur, glow, smooth text, animated focus, and controller navigation. ImGui is used for debug tools only.

## Rendering Style

The UI exposes a NanoVG-style API:

- Begin frame.
- Draw rounded rect.
- Draw path.
- Draw text.
- Draw image.
- Draw blur panel.
- Draw glow.
- Push clip.
- Push transform.

The backend is Vulkan-native, not a browser overlay.

## Text

Text requirements:

- SDF or MSDF text.
- Font atlas.
- Crisp scaling.
- Localization-ready string IDs later.
- No negative letter spacing.
- Text must fit controls at target resolutions.

## Navigation

Inputs:

- Mouse.
- Keyboard.
- Controller.

Focus model:

- Explicit focus graph for menus.
- Visual focus ring/glow.
- Back/cancel action.
- Confirm action.
- Slider/stepper support.

Current pointer state:

- NovaCore input snapshots now retain absolute pointer coordinates and short click press/release edges.
- `GameMenu` maps window-space pointer coordinates into the 1280x720 UI layout used by rendering.
- Main menu tabs and selectable rows are clickable for Play, Gamemodes, Loadout, Character, Settings, and Account.
- Pointer hover selects rows; primary click activates the same runtime action as keyboard/controller confirm.

## Menus

V1 screens:

- Main menu.
- Play/direct connect.
- Settings.
- Graphics.
- Audio.
- Input.
- Controller tuning.
- Loadout placeholder.
- Pause menu.

Current runtime screens:

- Main Menu shell.
- Play tab with Firing Range entry.
- Gamemodes tab with TDM and Control placeholders.
- Loadout tab with base weapon and six attachment slots.
- Character tab with operator/helmet/armor/rig placeholder rows.
- Settings tab with live mouse sensitivity, controller sensitivity, HUD scale, aim assist, and damage-number toggles.
- Account tab with K/D, win rate, best weapon, best operator, damage per match, and accuracy.
- Loading screen for Firing Range and placeholder modes.
- Dev Shooting Range HUD/debug screen.
- Menu-flow smoke route inside `nemisis_game` that touches Main Menu tabs, loading screens, Dev Range, TDM placeholder, and Control placeholder.

## HUD

HUD elements:

- Health.
- Ammo.
- Weapon state.
- Utility state.
- Crosshair.
- Hitmarker.
- Damage direction.
- Match score.
- Objective state.
- Killfeed.
- Network stats debug toggle.

Current debug/UI implementation:

- `UiCanvas` is now the game-owned NanoVG-style command layer for UI/HUD primitives.
- `UiCanvas` records semantic commands for rects, rounded rects, lines, text, progress bars, and crosshairs.
- `UiCanvas` now provides reusable immediate UI primitives for text metrics, fit-to-width text scaling, shadow/outline text, panels, selectable buttons, status pills, and dividers.
- `UiCanvas` records image placeholder commands so committed UI art paths can be wired before the final texture backend exists.
- The current bridge flushes `UiCanvas` commands into NovaCore debug primitives until the Vulkan-native vector/text backend exists.
- SDL debug render primitives remain available only through explicit legacy launch paths.
- `GameMenu` owns Main Menu tabs, Loading, Dev Shooting Range, Team Deathmatch placeholder, and Control placeholder screens.
- `GameMenu` exposes explicit screen-driving helpers so runtime code, smoke tests, and future match/session flow use the same UI layers.
- Main Menu tabs, selectable rows, HUD panels, loading panels, and debug panels now use the shared UI primitive style layer rather than one-off debug rectangles.
- The Dev Shooting Range HUD now shows weapon/ammo, active range score, accuracy, streaks, target HP, target respawn, player health, and short event feedback.
- The Gameplay debug page includes player HP and Dev Range score/accuracy in addition to movement and collision telemetry.
- `Q/E` and controller shoulders switch top-level menu tabs.
- Mouse clicks switch top-level menu tabs and activate selectable rows.
- Left/Right and controller D-pad left/right adjust Loadout and Settings values live.
- Keyboard `P` and controller `Y` reset the active Dev Shooting Range session while gameplay is active.
- `F1` toggles the debug overlay.
- `Tab` and controller Start/Menu cycle debug overlay pages.
- Debug pages currently cover Gameplay, Network, and Assets/Render telemetry.
- The Assets/Render page includes NovaCore backend frame telemetry: swapchain readiness/size, submitted and skipped frames, recreate count, and last world box/mesh/line counts.
- `nemisis_ui_canvas_tests` protects the command recording and flush path.
- `nemisis_game_menu_flow_smoke` protects the in-process menu/loading/range/mode flow.
- This is a temporary development UI bridge until the custom Vulkan NanoVG-style renderer replaces debug text and primitive flushing.

## Settings

Settings categories:

- Display.
- Graphics.
- Audio.
- MKB.
- Controller.
- Gameplay.
- Network/debug.

Settings persist to user config and apply predictably. Some settings apply immediately; renderer backend changes may require restart.

Current implemented settings:

- Mouse sensitivity X/Y.
- Mouse ADS multiplier.
- Mouse invert Y.
- Controller look sensitivity X/Y.
- Controller deadzones as parsed config values.
- Controller response curve string placeholder.
- Aim assist enable/slowdown/rotation values.
- Damage number visibility.
- HUD scale.
- Debug world-line visibility.
- Runtime persistence for the above player-facing settings plus the active loadout snapshot.

Current gap:

- Graphics/audio settings still need deeper categories, validation, and per-setting apply/restart policy.
- The final Vulkan-native text/vector UI backend is still pending; current UI records NanoVG-style commands and flushes through the debug primitive bridge.
- Menu automation currently uses deterministic smoke stages; final animated transitions and focus graph polish are still pending.

## Acceptance

UI is acceptable when:

- It looks modern at 1080p/1440p.
- Controller navigation works.
- Settings persist.
- HUD does not hide important targets.
- Debug UI remains separate from game UI.







