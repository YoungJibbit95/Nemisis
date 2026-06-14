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

## Current Runtime Backbone

The current runtime has a first procedural animation layer in `nemisis::player::PlayerAnimation`.
It is not the final skeletal animation system, but it gives the FPS viewmodel a persistent animation state now instead of keeping pose math buried in renderer-local helper code.

Current animation data flow:

- `GameApp` simulates movement, weapons, and the camera rig on the fixed tick.
- The fixed tick feeds `CharacterAnimationInput` with velocity, movement mode, `MovementTechState`, ADS/reload/recoil/fire state, camera roll, and mantle progress.
- `CharacterAnimationState` keeps locomotion phase, blend values, reload/fire transients, and the last observed shot index.
- `CharacterAnimationFrame` exposes the frame consumed by rendering: locomotion clip, upper-body clip, ADS/reload/fire alphas, wallrun lean, first-person weapon/body/arms offsets, right/left hand local offsets, support-elbow offset, and third-person body pose offsets.

Current supported clips/cues:

- Idle, walk, sprint, slide, airborne, wallrun, mantle.
- Reload, ADS, and fire upper-body overlays.
- Gravity-boot wallrun lean.
- Left-hand double-jump energy-platform throw cue.
- Mantle reach/climb cue.

The Dev Range now renders the local camera presentation from this frame:

- Full local third-person body is skipped when the FPS camera rig is active.
- The first-person Project character body is lowered/scaled as a temporary torso/legs proxy so the camera no longer sits inside the head.
- First-person arms, right-hand grip, left support hand, and support elbow submit as weapon-attached mesh anchors.
- Weapon/arms/body offsets respond to sprint, slide, wallrun, mantle, ADS, reload, recoil, fire, and double-jump platform cues.

Coverage:

- `nemisis_player_animation_tests` validates clip selection, movement poses, ADS damping, reload/fire overlays, energy-platform hand cue, and reset behavior.
- `nemisis_dev_range_render_scene_tests` validates first-person mesh submissions and that the FPS camera rig hides the complete local third-person body while preserving first-person body/arms/weapon meshes.

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

Until authored skeleton clips exist, `DevRangeRenderSceneBuilder` renders a hybrid of real imported meshes and small debug geometry for the same cues:

- Boot glow boxes when gravity inverters are active.
- First-person arms mesh for wall-run forearm control, double-jump throw, and mantle reach beats.
- Weapon-attached first-person hand/support anchors driven by `CharacterAnimationFrame`.
- A short-lived energy platform on double jump.
- Small muzzle/aim/boot/energy platform boxes where dedicated VFX assets do not exist yet.
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







