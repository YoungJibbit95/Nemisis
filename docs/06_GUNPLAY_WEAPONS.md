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

## Recoil Model

Use pattern + noise:

- Pattern gives learnable vertical/horizontal movement.
- Noise adds small organic variation.
- Recoil recovery is tunable.
- ADS modifies recoil.
- Movement modifies spread, not necessarily recoil.

The server validates shot timing and spread seed. The client predicts visual recoil immediately.

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

Code foundation:

- `nemisis::weapons::WeaponDefinition`
- `nemisis::weapons::DamageProfile`
- `nemisis::weapons::WeaponRuntimeState`
- `nemisis::weapons::FireRequest`
- `nemisis::weapons::FireResult`
- `nemisis::weapons::WeaponSystem`
- `WeaponSystem::loadFromConfig`
- `nemisis::weapons::simulateWeaponTick`

Config fields currently parsed:

- `id`
- `class`
- `display_name`
- `magazine_size`
- `fire_rate_rpm`
- `ads_time`
- `reload_time`
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







