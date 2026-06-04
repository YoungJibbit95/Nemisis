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

Third-person:

- Full body mesh.
- Locomotion animation.
- Aim offsets.
- Weapon sockets.
- Hitboxes.
- Network interpolation.

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

