# 10 Input Controller MKB

## Input Goal

MKB and controller are both first-class. The engine does not treat controller as a late port feature. Both inputs feed the same gameplay command model while preserving device-specific tuning.

## Input Layers

1. Platform events.
2. Raw device state.
3. Binding/action map.
4. Device-specific processing.
5. Gameplay command generation.

The current code foundation uses NovaCore `InputActionMap`, Nemisis-owned default action names/bindings in `nemisis::input`, and `InputCommandBuilder` for gameplay command generation.

## MKB

Support:

- Keyboard bindings.
- Mouse buttons.
- Raw mouse input where available.
- Horizontal/vertical sensitivity.
- ADS multiplier.
- Scoped multiplier.
- Toggle/hold options.

## Controller

Support:

- Gamepad detection.
- Button mapping through default A/B/X/LeftStick bindings.
- Left-stick movement axes.
- Trigger-based ADS/fire actions.
- Left/right stick deadzones in config.
- Axial and radial deadzone options.
- Response curves.
- Trigger thresholds.
- Vibration settings.
- Controller menu navigation.

## Aim Assist

Aim assist philosophy: competitive subtle.

Systems:

- Reticle slowdown near targets.
- Rotational assist only within tight rules.
- Range and FOV limits.
- Visibility checks.
- Separate tuning per weapon class if needed.

Rules:

- Aim assist must be data-driven.
- Debug visualization must show assist zones.
- It must be possible to disable assist for testing.
- MKB must not receive hidden aim correction.

## Fairness

Initial approach:

- Mixed input allowed.
- Assist values are conservative.
- Input source is tracked for telemetry.
- Future input-split matchmaking remains possible.

## Settings Persistence

Settings:

- Keybinds.
- Controller binds.
- Sensitivity.
- Deadzones.
- Curves.
- Aim assist toggle/intensity where allowed.
- Vibration.

Current persistence slice:

- Runtime mouse/controller look settings, HUD scale, damage-number visibility, aim-assist toggles, and active loadout are saved to `configs/user/nemisis_user.json`.
- Missing user settings fall back to config defaults.
- Invalid stored attachment ids are ignored during load so stale user files do not crash startup.

## Acceptance

Input is acceptable when:

- MKB and controller are detected.
- Actions are device-independent.
- Controller curves are tunable.
- Settings persist.
- Menus can be operated with controller.

Current implemented slice:

- MKB and controller bindings feed the same `InputActionMap`.
- Left stick maps to movement actions.
- Mouse delta maps to look actions.
- Right stick maps to look actions.
- Right trigger maps to fire.
- Left trigger maps to ADS.
- Gamepad button bindings cover jump, dash, slide, sprint, mantle, and reload.
- Keyboard `P` and controller `Y` map to `ResetRange` for the Dev Shooting Range.
- `F1` toggles the debug overlay.
- `Tab` cycles debug overlay pages.
- Controller Start/Menu cycles debug overlay pages.
- `Q/E` switch menu tabs for keyboard users.
- Controller shoulders switch menu tabs.
- Left/Right and controller D-pad left/right adjust live Loadout/Settings rows.
- Mouse/controller look sensitivity and ADS look scaling are applied by `InputCommandBuilder`.
- Runtime Settings and Loadout edits persist through `UserSettingsPersistence`.
- Aim assist config values are parsed and exposed, but no target slowdown/rotation assist is applied yet.
- Dev Shooting Range requests relative mouse mode while gameplay is active.
- Input command tests verify keyboard/mouse, controller command generation, settings-scaled look output, and shared action-map behavior.







