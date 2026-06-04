# 04 Player Movement

## Movement Goal

Movement is a core identity system. It should feel fast and expressive like an Apex-lite arena shooter while remaining server-validatable for competitive play.

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
- Crouch.
- Sprint.
- Slide.
- Mantle request.
- Fire/ADS/reload actions.
- Device source metadata.

MKB and controller produce the same command shape.

## Core States

- Grounded.
- Airborne.
- Sprinting.
- Sliding.
- Crouched.
- Mantling.
- Stunned/disabled.

State transitions must be deterministic enough for prediction/reconciliation.

## Movement Features

V1 planned features:

- Walk.
- Sprint.
- Crouch.
- Slide.
- Jump.
- Air control.
- Mantle.
- Advanced movement tech hooks.

Advanced tech examples:

- Momentum-preserving slide jumps.
- Timing-sensitive jump/slide transitions.
- Wall interaction hooks for later movement tech.
- Air acceleration caps that reward skill without breaking validation.

## Server Validation

The server validates:

- Maximum speed envelope.
- Acceleration envelope.
- Allowed state transitions.
- Cooldowns.
- Grounded/airborne claims.
- Mantle targets.

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

- Max walk speed.
- Sprint speed.
- Slide impulse.
- Slide friction.
- Air acceleration.
- Jump velocity.
- Mantle range and duration.
- Crouch height.
- Controller curves.

## Acceptance

Movement is acceptable when:

- It can be predicted locally.
- Server can replay commands.
- Reconciliation is bounded.
- MKB and controller both feel intentional.
- Movement tech is measurable instead of accidental.







