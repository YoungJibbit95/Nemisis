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

## Acceptance

UI is acceptable when:

- It looks modern at 1080p/1440p.
- Controller navigation works.
- Settings persist.
- HUD does not hide important targets.
- Debug UI remains separate from game UI.

