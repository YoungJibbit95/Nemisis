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
- `nemisis::movement::MovementTechState`
- `nemisis::movement::MovementTechCue`

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
- Wall-run contact uses NovaCore `PhysicsWorld` wall-run surface probes, stores wall normal/tangent, carries tangent velocity, preserves double jump, and supports the first wall-jump impulse.
- Wall-run entry now emits a movement-tech cue for the operator arm-button animation and gravity-inverter boot glow.
- Double-jump now emits a movement-tech cue for the left-hand energy-platform throw/step visual.
- Mantle input now checks NovaCore mantle probes against cover/ledge tops, snaps to a deterministic target foot position, and emits mantle-reach/mantle-climb animation/VFX cues.
- Movement state stores slide/dash timers, grounded/airborne time, input magnitude, and last horizontal speed for camera/HUD/debug use.
- The first `PlayerCameraRig` turns movement state into local visual camera feel: smoothed eye position, head bob, roll, FOV kick, weapon sway, and recoil view offsets.

Replay foundation:

- `nemisis_movement_replay_tests`
- Ground acceleration/friction replay.
- Sprint convergence replay.
- Jump -> double-jump replay.
- Dash cooldown replay.
- Slide duration and slide-jump momentum replay.
- Wall-run contact and wall-jump replay.
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

Implemented foundation:

- Walk.
- Sprint / Tactical Sprint (with stamina/cooldown).
- Slide (momentum-based).
- Jump / Double Jump.
- Dash (multi-directional, tactical cooldown).
- Wall Run contact entry and Wall Jump.
- Wall Run and Double Jump movement-tech visual cues.
- First mantle/climb foundation with engine-backed ledge probe, deterministic target snap, debug lines, and `mantle-climb` cue.

Planned features:

- Crouch.
- Mantle.
- Strafing / Air Strafing.
- Wall-run camera lean, detach rules, cooldowns, eligibility windows, and server replay validation.

## Nemisis Movement Tech Lore

Wall-running is not a hero ability or limited tactical power. In the Nemisis universe, operators use mature gravity-inverter boot technology, so any trained operator can wall-run whenever movement conditions allow it. Gameplay treats this as a normal KCC transition, while presentation emits a short first-person cue: the operator's left hand reaches across and presses a control on the right forearm, then the boots glow while wall contact remains valid.

Double-jumping uses a different piece of operator kit. The operator throws or projects a compact energy platform with the left hand, plants one mid-air step on it, then pushes off. Gameplay still treats this as a deterministic second jump with one available charge, but animation/VFX must represent the energy step instead of a generic booster jump.

Mantle and climb tech should read as physical operator traversal rather than magic. The current implementation uses NovaCore mantle probes to find cover/ledge tops in front of the player, validates a landing position, snaps the KCC to the target top as a first deterministic foundation, and emits mantle-reach/mantle-climb cues for first-person arms and future third-person animation.

Current code hooks:

- `triggerWallRunGravityTech`: starts the arm-button cue and gravity boot active state on wall-run entry.
- `keepGravityBootsActive`: keeps boot glow alive while wall contact continues.
- `stopGravityBoots`: clears wall-run tech when wall-running ends or the player lands.
- `triggerDoubleJumpPlatform`: spawns the energy-platform cue below the operator on double jump.
- `triggerWallJumpDetach`: emits a detach cue when jumping away from a wall.
- `triggerMantleReach`: emits the early mantle/climb reach cue.
- `triggerMantleClimb`: emits the first successful climb cue and stores the mantle target/normal for VFX and animation.
- `PhysicsWorld::probeMantle`: finds reachable cover/ledge tops and reports obstacle point, target foot position, approach normal, height, distance, and collider id.

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
