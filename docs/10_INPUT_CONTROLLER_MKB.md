# 10 Input Controller MKB

## Input Goal

MKB and controller are both first-class. The engine does not treat controller as a late port feature. Both inputs feed the same gameplay command model while preserving device-specific tuning.

## Input Layers

1. Platform events.
2. Raw device state.
3. Binding/action map.
4. Device-specific processing.
5. Gameplay command generation.

The current code foundation uses NovaCore `InputActionMap` and Nemisis-owned default action names/bindings in `nemisis::input`.

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
- Button mapping.
- Left/right stick deadzones.
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

## Acceptance

Input is acceptable when:

- MKB and controller are detected.
- Actions are device-independent.
- Controller curves are tunable.
- Settings persist.
- Menus can be operated with controller.







