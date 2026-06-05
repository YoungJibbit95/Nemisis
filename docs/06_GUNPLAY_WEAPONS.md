# 06 Gunplay Weapons

## Gunplay Goal

Gunplay should feel precise, fast, and readable. The target is BO-like short-to-mid TTK with learnable recoil and enough movement interaction to reward skill.

## V1 Weapon Roster

- Assault rifle: baseline mid-range weapon.
- SMG: close-range tracking and mobility weapon.
- Shotgun: close burst weapon.
- Sidearm: fallback precision weapon.

## TTK Targets

Initial target ranges:

- Close range: 250-450 ms.
- Mid range: 450-700 ms.
- Long range: 700-1000 ms.

TTK depends on armor/health model, headshot multiplier, fire rate, and damage falloff. The first combat slice should log measured TTK values against test targets.

Current measured baseline helpers:

- `WeaponMetrics::damageBandForDistance`
- `WeaponMetrics::damageAtDistance`
- `WeaponMetrics::shotsToEliminate`
- `WeaponMetrics::estimateTimeToKill`

The current 150 HP test baselines are:

- Prototype AR close body: 6 shots, about 417 ms at 720 RPM.
- Prototype AR close headshot-only estimate: 4 shots, about 250 ms at 720 RPM.
- Prototype SMG close body: 7 shots, about 400 ms at 900 RPM.

## Weapon State

Core design states:

- Idle.
- Firing.
- ADS.
- Reloading.
- Swapping.
- Empty.

State must be predictable locally but validated by server.

Current implementation state:

- `WeaponRuntimeState::weaponId`
- `WeaponRuntimeState::ammoInMagazine`
- `WeaponRuntimeState::shotIndex`
- `WeaponRuntimeState::fireCooldownRemaining`
- `WeaponRuntimeState::reloadTimeRemaining`
- `WeaponRuntimeState::reloading`

The first simulation slice is intentionally deterministic and render-free. It advances fire cooldowns, reload timers, ammo consumption, dry-fire throttling, and shot indices from fixed-tick requests.

Current shot trace data:

- Origin.
- Direction.
- Range.
- Damage.
- Spread degrees.
- Seed.
- Shot index.

`buildShotTrace` is the first hitscan-ready step. It does not resolve world collision yet; it produces deterministic trace data that client prediction and server validation can share.

## Recoil Model

Use pattern + noise:

- Pattern gives learnable vertical/horizontal movement.
- Noise adds small organic variation.
- Recoil recovery is tunable.
- ADS modifies recoil.
- Movement modifies spread, not necessarily recoil.

The server validates shot timing and spread seed. The client predicts visual recoil immediately.

Current implementation:

- Shot seed uses command tick.
- Shot index comes from `WeaponRuntimeState`.
- Spread combines hip/ADS weapon spread and movement speed.
- Recoil offset scales by shot index.
- Direction is normalized for future scene queries.

## Spread Model

Spread inputs:

- Hip-fire vs ADS.
- Movement speed.
- Airborne state.
- Crouch state.
- Weapon class.
- Recent shots.

Spread should never hide the core feel. The shooter should reward aim and recoil control.

## Damage Model

Damage zones:

- Head.
- Upper body.
- Lower body.
- Limbs.

Damage calculation:

1. Validate shot.
2. Resolve hit.
3. Apply zone multiplier.
4. Apply range falloff.
5. Apply armor/health rules if enabled.
6. Emit damage event.

## Weapon Data

Weapon data is hot-reloadable:

- Fire rate.
- Magazine size.
- Reload time.
- Damage.
- Falloff distances.
- Recoil pattern.
- Recoil noise.
- Spread values.
- ADS time.
- Movement multipliers.
- Max range.
- Hip spread.
- ADS spread.
- Recoil pitch per shot.
- Recoil yaw per shot.

Code foundation:

- `nemisis::weapons::WeaponDefinition`
- `nemisis::weapons::DamageProfile`
- `nemisis::weapons::WeaponRuntimeState`
- `nemisis::weapons::FireRequest`
- `nemisis::weapons::FireResult`
- `nemisis::weapons::ShotTraceRequest`
- `nemisis::weapons::ShotTraceResult`
- `nemisis::weapons::TtkEstimate`
- `nemisis::weapons::WeaponSystem`
- `WeaponSystem::loadFromConfig`
- `nemisis::weapons::simulateWeaponTick`
- `nemisis::weapons::buildShotTrace`
- `nemisis::weapons::estimateTimeToKill`

Config fields currently parsed:

- `id`
- `class`
- `display_name`
- `magazine_size`
- `fire_rate_rpm`
- `ads_time`
- `reload_time`
- `max_range`
- `spread.hip_degrees`
- `spread.ads_degrees`
- `recoil.pitch_per_shot_degrees`
- `recoil.yaw_per_shot_degrees`
- `damage.close`
- `damage.mid`
- `damage.long`
- `damage.head_multiplier`
- `pellets`
- `damage_per_pellet`

## Feedback

Client feedback:

- Muzzle flash.
- Tracer.
- Impact effect.
- Hitmarker.
- Damage number optional for debug only.
- Kill confirmation.
- Controller vibration settings.

## Acceptance

Gunplay is acceptable when:

- Core trio and sidearm are data-driven.
- Recoil is deterministic with seed control.
- TTK can be measured.
- Client feedback is instant.
- Server remains authoritative.







