# Blender Agent Briefs

These briefs are designed for a Blender-capable agent or a local Blender automation script. All output must be original project work and must follow `docs/16_ASSET_PRODUCTION_PLAN.md`.

## Brief 00 - Dev Player Capsule Proxy

Asset id: `chr_player_capsule_proxy_01`

Purpose: a capsule-ish local/remote player proxy for early movement, collision, hitbox, and replication tests.

Requirements:

- Scale: 1.8 m tall, roughly 0.75 m movement capsule diameter.
- Origin: feet/root at world base.
- Meshes: simple capsule body, forward-facing stripe, foot/base orientation marker.
- Hitbox guides: head, torso, pelvis.
- Collision: `col_chr_player_capsule_proxy_01` capsule proxy.
- Sockets: `socket_root`, `socket_camera`, `socket_weapon_root`, `socket_hit_head`, `socket_hit_torso`.
- Export: `assets/export/gltf/characters/chr_player_capsule_proxy_01.glb`.
- Preview: `assets/previews/chr_player_capsule_proxy_01.png`.

Acceptance:

- Fits current capsule movement assumptions.
- Forward direction and camera height are readable.
- Hitbox guide volumes are visible without final character art.

## Brief 01 - Dev Target Dummy

Asset id: `prop_target_dummy_01`

Purpose: a readable target dummy for early hitscan, damage, TTK, and aim testing.

Requirements:

- Scale: roughly 1.8 m tall, 0.65 m wide.
- Origin: floor center at the base.
- Meshes: body capsule, head zone, torso zone, optional base plate.
- Materials: neutral body, bright hit-zone accents, eliminated/damaged variant slots.
- Collision: `col_prop_target_dummy_01` simple sphere/capsule proxy.
- Sockets: `socket_hit_center`, `socket_head_center`, `socket_vfx`.
- Export: `assets/export/gltf/props/prop_target_dummy_01.glb`.
- Preview: `assets/previews/prop_target_dummy_01.png`.

Acceptance:

- Clearly visible from 10 m, 20 m, and 40 m.
- Head and torso zones are visually distinct.
- No copyrighted logos or recognizable copied designs.

## Brief 02 - Prototype AR

Asset id: `wpn_ar_01`

Purpose: first readable assault rifle for BO-like baseline gunfeel tests.

Requirements:

- Scale: plausible first-person and world scale, around 0.75 m long.
- Origin: grip/root area suitable for weapon socket attachment.
- Meshes: receiver, barrel, magazine, grip, stock, sight blockout.
- Materials: dark polymer, brushed metal, small neutral accent color.
- Sockets: `socket_muzzle`, `socket_grip_r`, `socket_grip_l`, `socket_eject`, `socket_vfx`.
- LODs: `wpn_ar_01_lod0`, `wpn_ar_01_lod1`.
- Export: `assets/export/gltf/weapons/wpn_ar_01.glb`.
- Preview: `assets/previews/wpn_ar_01.png`.

Acceptance:

- Silhouette reads as AR, not SMG or shotgun.
- Muzzle socket points along gameplay forward direction.
- Magazine and grip are large enough for animation reference.

## Brief 03 - Movement Test Arena Kit

Asset id prefix: `env_test_`

Purpose: modular pieces for a small arena where movement, gunfeel, and server prediction can be tested.

Requirements:

- Scale: snap to 1 m grid.
- Pieces: floor grid, wall segment, low cover, high cover, slide ramp, launch ramp, stairs, mantle ledges, wall-run panels, doorway frame, spawn markers, control marker, distance markers.
- Origins: centered or grid-friendly pivots.
- Materials: neutral concrete/metal, movement surfaces with clear accent strips.
- Collision: simple `col_` mesh per piece.
- Export folder: `assets/export/gltf/environments`.
- Preview folder: `assets/previews`.

Acceptance:

- Pieces can form a simple 6v6 lane/control-point map.
- Mantle ledges exist at 0.8 m, 1.2 m, and 1.6 m.
- Slide ramp angle and wall-run panels are visually obvious.

## Brief 04 - First-Person Arms Proxy

Asset id: `chr_dev_arms_a`

Purpose: early first-person animation and weapon socket testing.

Requirements:

- Scale: human arms matched to weapon grip positions.
- Origin: camera/root alignment point.
- Meshes: left arm, right arm, simple glove/sleeve geometry.
- Skeleton: stable arm bones with readable names.
- Sockets: `socket_camera`, `socket_weapon_root`, `socket_hand_r`, `socket_hand_l`.
- Materials: neutral sleeve, glove, skin placeholder or covered hands.
- Export: `assets/export/gltf/characters/chr_dev_arms_a.glb`.
- Preview: `assets/previews/chr_dev_arms_a.png`.

Acceptance:

- Arms do not block the center aim line too aggressively.
- Hands align with `wpn_ar_01` grip sockets.
- No final IK or final animation polish required.

## Brief 05 - Third-Person Player Proxy

Asset id: `chr_dev_soldier_a`

Purpose: first remote/local player proxy for replication, hitbox, and animation tests.

Requirements:

- Scale: 1.8 m tall, readable competitive silhouette.
- Origin: feet/root at world base.
- Meshes: body, helmet/head, torso, arms, legs, simple gear.
- Skeleton: stable humanoid bone names.
- Hitbox guides: head, torso, pelvis, upper/lower arms, upper/lower legs.
- Materials: neutral team-ready base material plus accent slots.
- Export: `assets/export/gltf/characters/chr_dev_soldier_a.glb`.
- Preview: `assets/previews/chr_dev_soldier_a.png`.

Acceptance:

- Clear head/torso/limb readability.
- Fits current capsule movement assumptions.
- Rig and hitbox guides are ready for later animation work.

## Brief 06 - Prototype SMG

Asset id: `wpn_smg_01`

Purpose: compact weapon blockout for high-RPM close/mid-range gunfeel tests.

Requirements:

- Scale: shorter than `wpn_ar_01`, around 0.5 m long.
- Origin: grip/root area suitable for weapon socket attachment.
- Meshes: compact receiver, short barrel, box magazine, grip, stock stub, front stop/top rail.
- Materials: dark polymer, brushed metal, neutral teal accent.
- Sockets: `socket_muzzle`, `socket_grip_r`, `socket_grip_l`, `socket_eject`, `socket_vfx`.
- LODs: `wpn_smg_01_lod0`, `wpn_smg_01_lod1`.
- Export: `assets/export/gltf/weapons/wpn_smg_01.glb`.
- Preview: `assets/previews/wpn_smg_01.png`.

Acceptance:

- Silhouette reads compact and distinct from the AR.
- Muzzle and grip sockets are easy to inspect.
- Magazine and front support are large enough for animation reference.

## Brief 07 - Prototype Sidearm

Asset id: `wpn_sidearm_01`

Purpose: fallback pistol blockout for first-person and world-model attachment tests.

Requirements:

- Scale: compact pistol silhouette, around 0.35 m long.
- Origin: grip/root area suitable for weapon socket attachment.
- Meshes: slide, frame, grip, barrel, trigger guard, front/rear sights.
- Materials: dark polymer, metal slide, small orange accent.
- Sockets: `socket_muzzle`, `socket_grip_r`, `socket_grip_l`, `socket_eject`, `socket_vfx`.
- LODs: `wpn_sidearm_01_lod0`, `wpn_sidearm_01_lod1`.
- Export: `assets/export/gltf/weapons/wpn_sidearm_01.glb`.
- Preview: `assets/previews/wpn_sidearm_01.png`.

Acceptance:

- Sidearm reads clearly at first-person and pickup scale.
- Slide, grip, and muzzle are visually distinct.
- Uses original primitive geometry only.
