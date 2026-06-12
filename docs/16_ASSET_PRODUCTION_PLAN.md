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

- Blender is available on this machine through an explicit path: `F:\Program Files\Blender Foundation\Blender 5.1\blender.exe`.
- Blender is not currently in PATH, so call the helper with `-BlenderPath` or add the Blender folder to PATH.
- A0 dev primitives have been generated as `.blend`, `.glb`, and metadata files under `assets/source/blender` and `assets/export/gltf`.
- A prototype-pack generator has also produced lightweight GLBs under `assets/generated/prototype_pack` for fast render/greybox iteration.
- A1 shooter prototype assets have been generated through `tools/blender/make_a1_prototype_pack.py`.

The first generator script is `tools/blender/make_dev_primitives.py`. It creates source `.blend` files, glTF exports, and metadata for the target dummy, player capsule proxy, first-person arms proxy, third-person soldier proxy, AR/SMG/sidearm blockouts, and movement test arena kit.

The second generator script is `tools/blender/make_prototype_pack.py`. It creates a small procedural pack for an SMG, humanoid, wall panel, floor tile, cover crate, ramp, and target stand. These assets are cataloged as required Dev Sandbox renderables so NovaCore's GLB import and Vulkan mesh upload path can be tested quickly.

The A1 generator script is `tools/blender/make_a1_prototype_pack.py`. It creates original, generic blockout assets inspired by the requested weapon/operator direction without copying protected designs or using brand names:

- `wpn_a1_compact_rifle_01`: compact 300BLK-style rifle direction.
- `wpn_a1_modern_rifle_01`: modern modular assault-rifle direction.
- `wpn_a1_compact_sidearm_01`: compact striker-fired sidearm direction.
- `chr_a1_stylized_operator_01`: US soldier + sci-fi pilot/operator hybrid.
- `chr_a1_fp_arms_01`: first-person arms for weapon-hold tests.

Run command:

```powershell
& "F:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --python tools\blender\make_a1_prototype_pack.py
```

Nemisis runtime asset ids are declared in `configs/assets/nemisis_assets.json`. This catalog is loaded by `GameAssetCatalog` and mounted through NovaCore's asset manifest/registry backbone.

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
  generated/
    prototype_pack/
  textures/
  materials/
  collision/
  previews/
```

Rules:

- `.blend` source files live under `assets/source/blender`.
- Runtime glTF exports live under `assets/export/gltf`.
- Fast procedural prototype-pack exports live under `assets/generated/prototype_pack`.
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
- `wpn_ar_01`: first readable weapon blockout with muzzle socket.
- `wpn_smg_01`: compact weapon blockout with grip and muzzle sockets.
- `wpn_sidearm_01`: fallback sidearm blockout with grip and muzzle sockets.
- `env_test_arena_kit_01`: metric floor grid, walls, cover, ramps, spawn markers, distance markers, and movement-test pieces.

Acceptance:

- Scale matches gameplay meters.
- Silhouettes are readable in a greybox scene.
- Weapon has muzzle and grip sockets.
- Target dummy origin and collision proxy are usable for hit tests.

Current generated outputs:

- `assets/source/blender/props/prop_target_dummy_01.blend`
- `assets/source/blender/characters/chr_player_capsule_proxy_01.blend`
- `assets/source/blender/characters/chr_dev_arms_a.blend`
- `assets/source/blender/characters/chr_dev_soldier_a.blend`
- `assets/source/blender/weapons/wpn_ar_01.blend`
- `assets/source/blender/weapons/wpn_smg_01.blend`
- `assets/source/blender/weapons/wpn_sidearm_01.blend`
- `assets/source/blender/environments/env_test_arena_kit_01.blend`
- Matching `.glb` exports and `.metadata.json` files under `assets/export/gltf`.
- Prototype-pack GLBs plus metadata under `assets/generated/prototype_pack`.
- Prototype-pack assets are now included in `configs/assets/nemisis_assets.json` and validated by `DevAssetBindings`.
- A1 prototype-pack `.blend`, `.glb`, and metadata files for compact rifle, modern rifle, compact sidearm, stylized operator, and first-person arms.
- A1 assets are now included in `configs/assets/nemisis_assets.json`, required by `DevAssetBindings`, imported into NovaCore CPU mesh data, registered as renderer mesh resources, and uploaded into the Vulkan Dev Range smoke path.

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

Codex/agent handoff files:

- `tools/blender/CODEX_ASSET_AGENT.md`
- `tools/blender/asset_job_template.json`
- `tools/blender/run_make_dev_primitives.ps1`

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

After Blender or a Blender path is available:

1. `tools/blender/make_dev_primitives.py`: generate the first simple target dummy, player/arms/soldier proxies, AR/SMG/sidearm blockouts, floor grid, and movement blockout kit.
2. `tools/blender/run_make_dev_primitives.ps1`: find Blender or accept `-BlenderPath` and run the generator from PowerShell.
3. `tools/blender/validate_asset.py`: verify naming, scale, transforms, sockets, and collision proxy names.
4. `tools/blender/export_gltf.py`: export selected source file to the expected glTF path.
5. `tools/blender/render_preview.py`: render a stable turntable or front/side preview.

These scripts should run headless so Codex can call them from the terminal and commit generated source/export files only after validation.
