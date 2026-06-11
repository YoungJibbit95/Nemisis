# 04 Player Movement

## Movement Goal

Movement is a core identity system. It should feel fast, fluid, and highly expressive, blending the best of Titanfall, Apex Legends, and Black Ops mechanics, while remaining server-validatable for competitive play.

## Controller Model

Use a custom kinematic character controller:

- Capsule shape.
- Ground detection.
- Slope handling.
- Step-up logic.
- Swept collision.
- Raycast probes.
- Trigger overlaps.

Rigid body physics is not required for player movement v1.

## Input Model

Movement consumes a normalized `PlayerInputCommand`:

- Tick number.
- Move vector.
- Look delta.
- Jump.
- Double Jump.
- Crouch/Slide.
- Sprint / Tactical Sprint.
- Dash.
- Mantle request.
- Fire/ADS/reload actions.
- Device source metadata.

MKB and controller produce the same command shape.
Look input updates `PlayerViewComponent`. Movement input is transformed relative to player yaw before `MovementSystem` consumes the command.

Code foundation:

- `nemisis::player::PlayerInputCommand`
- `nemisis::player::PlayerCommandQueue`
- `nemisis::player::PlayerIdentityComponent`
- `nemisis::player::PlayerNetworkComponent`
- `nemisis::player::PlayerViewComponent`
- `nemisis::movement::MovementTuning`
- `nemisis::movement::PlayerMovementState`
- `nemisis::movement::MovementSystem`

Runtime placement:

- Local player movement state is stored as a NovaCore ECS component.
- Fixed ticks push commands into `PlayerCommandQueue` before simulation.
- Fixed ticks apply player look and transform movement relative to view yaw.
- Simulated movement position is mirrored into the player's `TransformComponent`.
- Pending command count and last processed tick are tracked for future server acknowledgement.
- `MovementSystem` now accelerates toward target velocity instead of snapping instantly.
- Ground friction decelerates released input.
- Air movement has configurable acceleration, max speed, and drag.
- Slide has configurable impulse, duration, steering, friction, end speed, cooldown, and slide-jump boost.
- Dash has configurable impulse, steering, duration, and cooldown.
- Movement state stores slide/dash timers, grounded/airborne time, input magnitude, and last horizontal speed for camera/HUD/debug use.
- The first `PlayerCameraRig` turns movement state into local visual camera feel: smoothed eye position, head bob, roll, FOV kick, weapon sway, and recoil view offsets.

Replay foundation:

- `nemisis_movement_replay_tests`
- Ground acceleration/friction replay.
- Sprint convergence replay.
- Jump -> double-jump replay.
- Dash cooldown replay.
- Slide duration and slide-jump momentum replay.
- `nemisis_player_command_queue_tests`
- `nemisis_player_spawn_tests`
- `nemisis_player_camera_rig_tests`

## Core States

- Grounded.
- Airborne.
- Sprinting / Tactical Sprinting.
- Sliding.
- Crouched.
- Mantling.
- Wall Running.
- Stunned/disabled.

State transitions must be deterministic enough for prediction/reconciliation.

## Movement Features

Planned features:

- Walk.
- Sprint / Tactical Sprint (with stamina/cooldown).
- Crouch.
- Slide (momentum-based).
- Jump / Double Jump.
- Dash (multi-directional, tactical cooldown).
- Wall Run (geometry-locked, allows Wall Jump).
- Mantle.
- Strafing / Air Strafing.

Advanced tech requirements:

- Momentum-preserving slide jumps.
- Fluid chaining of Wall Run -> Wall Jump -> Double Jump.
- Timing-sensitive transitions.
- Air acceleration caps that reward skill without breaking validation.

## Server Validation

The server validates:

- Maximum speed envelope (including Dash/Tactical Sprint bursts).
- Acceleration envelope.
- Allowed state transitions.
- Cooldowns (Dash, Tactical Sprint).
- Grounded/airborne claims.
- Wall Run and Mantle targets.

Validation should correct impossible states without punishing normal client prediction drift.

## Controller Feel

Controller movement needs:

- Deadzone processing.
- Response curve.
- Analog sprint behavior rules.
- Consistent slide/mantle activation.
- Separate look curve and movement curve.

MKB needs:

- Raw mouse input where platform allows.
- Independent horizontal/vertical sensitivity.
- Optional scoped sensitivity multiplier.

## Replay Tests

Movement replay tests record input commands and expected positions. These tests are important because advanced tech must remain stable across patches.

## Tuning Data

Movement tuning should live in hot-reload data:

- Max walk speed / Sprint speed / Tactical Sprint speed.
- Dash impulse / cooldown.
- Slide impulse / friction.
- Slide duration / steering / end speed / slide-jump boost.
- Air acceleration / Air strafe capability.
- Ground acceleration / friction / stop speed.
- Jump velocity / Double Jump impulse.
- Wall Run speed / duration / friction.
- Mantle range and duration.
- Crouch height.
- Controller curves.

## Acceptance

Movement is acceptable when:

- It can be predicted locally.
- Server can replay commands.
- Reconciliation is bounded.
- MKB and controller both feel intentional.
- Movement tech (Wall Runs, Dashes, Slides) is measurable and consistent.


