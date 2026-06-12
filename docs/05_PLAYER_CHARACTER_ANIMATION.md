# 05 Player Character Animation

## Animation Goal

The game uses full third-person characters while keeping first-person feel responsive. First-person weapon/arms animation and third-person body animation are related but separate presentation layers.

## Character Representations

First-person:

- Arms.
- Weapon mesh.
- Camera-space animation.
- Recoil camera and weapon offsets.
- Reload and inspect animations.
- Wall-run gravity control press: left hand crosses to the right forearm and taps the boot-inverter control.
- Double-jump energy step: left hand throws/projects a compact platform under the player's feet before the second jump impulse.
- Mantle reach: left hand reaches forward/up toward the ledge while the KCC prepares a future mantle attach.

Third-person:

- Full body mesh.
- Locomotion animation.
- Aim offsets.
- Weapon sockets.
- Hitboxes.
- Network interpolation.
- Readable gravity-boot glow while wall-running.
- Readable air-step platform VFX on double jump.
- Wall-jump detach and mantle-reach body poses.

## Import Path

Marketplace or custom characters are cleaned in Blender and exported as glTF:

- Skeleton normalized.
- Mesh scale checked.
- Animation clips named consistently.
- Weapon sockets exported as nodes.
- Hitbox authoring helpers added where needed.

## M1-M3 Placeholder

Before real animation exists:

- Remote players can use simple proxy meshes.
- Hitboxes can be capsules.
- Weapon sockets can be debug transforms.
- Movement-tech cues can be rendered as debug meshes/VFX boxes until authored clips exist.

## Animation System Phases

Phase 1:

- Skeleton data.
- Clip sampling.
- Local-to-model pose calculation.

Phase 2:

- Blend trees.
- Additive aim offsets.
- Animation events.
- Socket transforms.

Phase 3:

- State machines.
- Upper/lower body layering.
- Foot placement.
- Basic IK.

Phase 4:

- Retargeting.
- Motion matching or advanced locomotion research.
- Procedural animation polish.

## Gameplay Links

Animation must provide:

- Muzzle socket.
- Left/right hand sockets.
- Camera event hooks.
- Reload event timings.
- Hitbox pose data.
- Third-person readable movement states.
- Movement-tech event hooks from `MovementTechState`:
  - `WallRunGravityArmTrigger` starts the first-person right-arm button press.
  - `GravityBootsActive` keeps boot glow and third-person wall-run readability alive.
  - `DoubleJumpEnergyPlatform` starts the left-hand platform throw and air-step VFX.
  - `WallJumpDetach` starts detach/brace animation.
  - `MantleReach` starts the ledge-reach pose.
  - `MantleClimb` starts the successful ledge pull-up/climb beat and carries the target position/normal.

The movement-tech hooks are presentation cues, not ability gates. Movement remains controlled by KCC state, surface probes, and deterministic input commands.

## Current Placeholder Visuals

Until authored skeleton clips exist, `DevRangeRenderSceneBuilder` renders small debug geometry for the same cues:

- Boot glow boxes when gravity inverters are active.
- A forearm control light on wall-run entry.
- A short-lived energy platform on double jump.
- A left-hand reach marker when mantle input is requested in air.
- A ledge-top highlight when the first mantle/climb snap succeeds.

These are intentionally simple but they lock down timing, naming, and gameplay integration before animation assets are final.

## Network Considerations

The server does not need full animation playback for every cosmetic detail, but it does need authoritative hitbox state or a deterministic approximation for rewind hit validation.

Client remote animation uses:

- Replicated movement state.
- Replicated aim pitch/yaw.
- Replicated weapon state.
- Interpolated transform history.

## Acceptance

Animation foundation is acceptable when:

- A skeleton loads.
- A clip samples.
- A pose can drive a mesh.
- A socket transform can be queried.
- Hitbox transforms can be generated from pose data.







