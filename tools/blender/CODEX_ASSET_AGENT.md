# Codex Blender Asset Agent

## Purpose

Use this guide when a Codex/Blender-capable agent is asked to create Nemisis assets.

The agent must create original assets only. It must not copy Apex, Call of Duty, Titanfall, Source, Unreal sample content, or unclear-license marketplace files.

## Inputs

Start with:

- `docs/16_ASSET_PRODUCTION_PLAN.md`
- `assets/briefs/BLENDER_AGENT_BRIEFS.md`
- `configs/assets/nemisis_assets.json`

The asset catalog is the runtime contract. If a generated asset id is not in `configs/assets/nemisis_assets.json`, add it there before the runtime should expect it.

## Expected Agent Workflow

1. Read the relevant brief.
2. Create or modify the Blender source asset under `assets/source/blender`.
3. Use meters and keep transforms applied.
4. Add required sockets as empties.
5. Add collision proxies with `col_` prefixes when the brief asks for them.
6. Export glTF/GLB under `assets/export/gltf`.
7. Write metadata JSON next to the export.
8. Update `configs/assets/nemisis_assets.json` if a new runtime asset id is introduced.
9. Run JSON and script syntax checks.
10. Commit source, export, metadata, and catalog changes together.

## Current Runnable Automation

The prepared generator creates first dev primitives:

```powershell
.\tools\blender\run_make_dev_primitives.ps1 -Only all
```

Direct Blender call:

```powershell
blender --background --python tools/blender/make_dev_primitives.py -- --only all
```

Useful `--only` values:

- `all`
- `target`
- `characters`
- `player`
- `arms`
- `soldier`
- `weapons`
- `ar`
- `smg`
- `sidearm`
- `arena`

The generator currently emits dev geometry for:

- `prop_target_dummy_01`
- `chr_player_capsule_proxy_01`
- `chr_dev_arms_a`
- `chr_dev_soldier_a`
- `wpn_ar_01`
- `wpn_smg_01`
- `wpn_sidearm_01`
- `env_test_arena_kit_01`

The A1 prototype-pack generator creates original visual blockouts for a compact rifle, modern rifle, compact sidearm, stylized operator, and optional first-person arms:

```powershell
blender --background --python tools/blender/make_a1_prototype_pack.py
```

Useful `--only` values:

- `all`
- `weapons`
- `characters`
- `compact_rifle`
- `modern_rifle`
- `sidearm`
- `operator`
- `arms`

## Acceptance Before Commit

- Source `.blend` exists.
- Export `.glb` exists.
- Metadata JSON exists.
- Asset id matches `configs/assets/nemisis_assets.json`.
- No unclear-license files were added.
- Required sockets are present.
- Collision proxies are named.
- Scale is in meters.
