# 16 Asset Production Plan

## Goal

This document turns Nemisis asset work into small, agent-ready Blender tasks. It defines what must be made, how it must be exported, and how each asset is accepted before it enters runtime testing.

The visual references are modern movement shooters and readable military/sci-fi FPS games, but the assets must be original. Do not copy Apex, Call of Duty, Titanfall, Source, Unreal sample content, or marketplace assets with unclear licensing.

## Tooling Position

Codex/GPT can help with Blender work when one of these exists in the workspace:

- Blender CLI installed and exposed through `blender` or `blender.exe`.
- A Blender Python automation script that can run headless.
- A Blender MCP/plugin/connector that exposes scene editing and export tools.

Current local environment:

- Blender is not installed or not visible in PATH.
- No Blender executable was found under the usual `C:\Program Files` install path.
- This block therefore implements the production plan and asset briefs, not generated `.blend` files.

The first generator script is already prepared in `tools/blender/make_dev_primitives.py`. Once Blender is available, it can create source `.blend` files, glTF exports, and metadata for the target dummy, AR blockout, and movement test arena kit.

## Art Direction

Nemisis should read as a competitive sci-fi arena shooter:

- Clean silhouettes for players, weapons, cover, ramps, doors, mantles, and objectives.
- Slightly grounded material language: metal, polymer, glass, rubber, fabric, painted concrete.
- High gameplay readability over visual noise.
- Mid-contrast arenas with strong team/objective color accents.
- Weapons with clear class silhouettes: AR stable, SMG compact, shotgun heavy, sidearm readable.
- Movement surfaces must visually explain use: slide ramps, mantle ledges, wall-run strips, jump pads, ziplines.

## Coordinate And Scale Rules

Source files:

- Blender source stays in Blender's native Z-up workflow.
- Asset scale uses meters.
- Keep object origins meaningful: floor pieces at local center, weapons at grip/root, characters at feet/root.
- Apply transforms before export unless the asset brief explicitly says otherwise.

Export:

- Export glTF 2.0 as `.glb` for runtime tests unless a multi-file `.gltf` is needed.
- Exported assets must be converted by the importer into NovaCore's runtime coordinate conventions.
- Engine gameplay currently uses X/Z horizontal movement and Y as vertical height.
- Any import conversion must be documented in asset metadata so movement, sockets, and collision stay predictable.

## Directory Layout

```text
assets/
  briefs/
    BLENDER_AGENT_BRIEFS.md
  source/
    blender/
      characters/
      environments/
      props/
      weapons/
  export/
    gltf/
      characters/
      environments/
      props/
      weapons/
  textures/
  materials/
  collision/
  previews/
```

Rules:

- `.blend` source files live under `assets/source/blender`.
- Runtime glTF exports live under `assets/export/gltf`.
- Preview renders live under `assets/previews`.
- Do not commit generated junk caches.
- Do not add unclear-license marketplace content.

## Naming Rules

Use stable lowercase ids:

- Weapons: `wpn_ar_01`, `wpn_smg_01`, `wpn_shotgun_01`, `wpn_sidearm_01`.
- Characters: `chr_dev_soldier_a`, `chr_dev_arms_a`.
- Environment kit: `env_arena_wall_01`, `env_arena_ramp_slide_01`, `env_arena_cover_low_01`.
- Props: `prop_target_dummy_01`, `prop_spawn_pad_01`.
- Collision meshes: `col_<asset_id>`.
- LOD meshes: `<asset_id>_lod0`, `<asset_id>_lod1`, `<asset_id>_lod2`.
- Sockets/empties: `socket_muzzle`, `socket_grip_r`, `socket_grip_l`, `socket_camera`, `socket_vfx`.

## Asset Metadata

Every production asset should ship with metadata next to the export:

```json
{
  "id": "wpn_ar_01",
  "source": "assets/source/blender/weapons/wpn_ar_01.blend",
  "export": "assets/export/gltf/weapons/wpn_ar_01.glb",
  "category": "weapon",
  "scale_meters": true,
  "runtime_up_axis": "Y",
  "gameplay_forward_axis": "+Z",
  "sockets": ["socket_muzzle", "socket_grip_r", "socket_grip_l"],
  "collision": "none",
  "lods": ["lod0", "lod1"],
  "license": "original_project_asset"
}
```

## Production Phases

### A0 - Dev Primitives

Purpose: unblock gameplay testing before final art.

Assets:

- `prop_target_dummy_01`: sphere/capsule target with visible hit zones.
- `chr_player_capsule_proxy_01`: simple player proxy matching movement collision.
- `wpn_ar_blockout_01`: first readable weapon blockout with muzzle socket.
- `env_test_floor_grid_01`: metric floor grid with 1 m and 5 m markings.
- `env_test_wall_mantle_01`: ledge pieces for mantle and jump tests.

Acceptance:

- Scale matches gameplay meters.
- Silhouettes are readable in a greybox scene.
- Weapon has muzzle and grip sockets.
- Target dummy origin and collision proxy are usable for hit tests.

### A1 - Shooter Test Field Kit

Purpose: build the first playable dev map around movement and gunfeel.

Assets:

- Low cover, high cover, corridor wall, ramp, slide ramp, stairs, doorway frame.
- Mantle ledge pieces at 0.8 m, 1.2 m, and 1.6 m.
- Wall-run panel strip with clear material accent.
- Spawn pads and control-point marker.
- Distance markers for 5 m, 10 m, 20 m, and 40 m weapon tests.

Acceptance:

- Pieces snap to a 1 m grid.
- Collision proxies are simple and named.
- Surfaces have metadata tags for slide, mantle, wall-run, and objective.
- The kit can form a small 6v6 test arena without visual confusion.

### A2 - Weapon Prototype Set

Purpose: get the core weapon classes into first-person tests.

Assets:

- `wpn_ar_01`: stable baseline rifle.
- `wpn_smg_01`: compact high-RPM weapon.
- `wpn_shotgun_01`: heavy close-range weapon.
- `wpn_sidearm_01`: readable fallback pistol.

Acceptance:

- Each weapon has a unique silhouette.
- Required sockets exist: muzzle, right grip, left support, casing/ejection optional.
- First-person and world scale are both checked.
- Materials are simple PBR and do not rely on copyrighted branding.

### A3 - Character And Arms Prototype

Purpose: prepare animation and hitbox work without waiting for final characters.

Assets:

- `chr_dev_soldier_a`: full third-person body, neutral rig, clean silhouette.
- `chr_dev_arms_a`: first-person arms, simple gloves/sleeves, weapon grip pose.
- Capsule/hitbox guide meshes for head, torso, pelvis, upper/lower limbs.

Acceptance:

- Skeleton uses stable bone names.
- Body and first-person arms are separate assets.
- Hitbox guides line up with the mesh.
- No final IK system is required yet, but sockets must exist.

### A4 - Gameplay Readability Pass

Purpose: make the slice easier to read before final rendering.

Assets:

- Team color material variants.
- Objective and spawn indicators.
- Muzzle flash placeholder mesh/VFX cards.
- Impact decal placeholders.
- Damage target state variants.

Acceptance:

- Color accents stay readable without overwhelming the arena.
- Effects are short, clear, and tunable.
- HUD/UI work can refer to stable asset ids.

## Blender Agent Brief Format

Each Blender agent task should include:

- Asset id and category.
- One-sentence gameplay purpose.
- Exact scale and origin requirements.
- Mesh list and naming rules.
- Socket/empty requirements.
- Collision proxy requirements.
- Material ids and texture constraints.
- LOD budget.
- Export path.
- Preview render path.
- Acceptance checklist.

The reusable briefs live in `assets/briefs/BLENDER_AGENT_BRIEFS.md`.

## Validation Checklist

Before an asset enters runtime tests:

- Source `.blend` exists.
- Exported `.glb` exists.
- Preview render exists.
- Scale is in meters.
- Transforms are applied.
- Names match this document.
- Required sockets exist.
- Collision proxy exists when requested.
- Materials are named and PBR-compatible.
- License is `original_project_asset` or otherwise documented.
- No copyrighted logos, copied game designs, or unclear marketplace files.

## First Automation Targets

When Blender is installed:

1. `tools/blender/make_dev_primitives.py`: generate the first simple target dummy, AR blockout, floor grid, and movement blockout kit.
2. `tools/blender/validate_asset.py`: verify naming, scale, transforms, sockets, and collision proxy names.
3. `tools/blender/export_gltf.py`: export selected source file to the expected glTF path.
4. `tools/blender/render_preview.py`: render a stable turntable or front/side preview.

These scripts should run headless so Codex can call them from the terminal and commit generated source/export files only after validation.
