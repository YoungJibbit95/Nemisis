# Blender Tools

These scripts are meant to run inside Blender, not plain Python.

Current local environment note:

- `blender` is not available in PATH.
- Blender CLI is available at `F:\Program Files\Blender Foundation\Blender 5.1\blender.exe`.
- The helper script searches PATH, normal Program Files locations, and the repo drive's Program Files folders.

## Generate Dev Primitives

From the Nemisis repository root:

```powershell
blender --background --python tools/blender/make_dev_primitives.py
```

Using the helper script:

```powershell
.\tools\blender\run_make_dev_primitives.ps1 -Only all
```

If Blender is not in PATH and auto-discovery misses it:

```powershell
.\tools\blender\run_make_dev_primitives.ps1 -Only all -BlenderPath "F:\Program Files\Blender Foundation\Blender 5.1\blender.exe"
```

Generate one asset group:

```powershell
blender --background --python tools/blender/make_dev_primitives.py -- --only target
blender --background --python tools/blender/make_dev_primitives.py -- --only characters
blender --background --python tools/blender/make_dev_primitives.py -- --only weapons
blender --background --python tools/blender/make_dev_primitives.py -- --only arena
```

Useful single-asset groups:

```powershell
blender --background --python tools/blender/make_dev_primitives.py -- --only player
blender --background --python tools/blender/make_dev_primitives.py -- --only arms
blender --background --python tools/blender/make_dev_primitives.py -- --only soldier
blender --background --python tools/blender/make_dev_primitives.py -- --only ar
blender --background --python tools/blender/make_dev_primitives.py -- --only smg
blender --background --python tools/blender/make_dev_primitives.py -- --only sidearm
```

The script writes:

- `.blend` source files under `assets/source/blender`.
- `.glb` exports under `assets/export/gltf`.
- Metadata JSON next to each export.

Generated dev primitive asset ids:

- `prop_target_dummy_01`
- `chr_player_capsule_proxy_01`
- `chr_dev_arms_a`
- `chr_dev_soldier_a`
- `wpn_ar_01`
- `wpn_smg_01`
- `wpn_sidearm_01`
- `env_test_arena_kit_01`

The generated assets are dev primitives only. They are meant to unblock gameplay testing, scale checks, sockets, and importer work before final art.

## Generate A1 Prototype Pack

From the Nemisis repository root:

```powershell
blender --background --python tools/blender/make_a1_prototype_pack.py
```

If Blender is not in PATH:

```powershell
& "F:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --python tools/blender/make_a1_prototype_pack.py
```

Generate one A1 group:

```powershell
blender --background --python tools/blender/make_a1_prototype_pack.py -- --only weapons
blender --background --python tools/blender/make_a1_prototype_pack.py -- --only characters
blender --background --python tools/blender/make_a1_prototype_pack.py -- --only compact_rifle
blender --background --python tools/blender/make_a1_prototype_pack.py -- --only modern_rifle
blender --background --python tools/blender/make_a1_prototype_pack.py -- --only sidearm
blender --background --python tools/blender/make_a1_prototype_pack.py -- --only operator
blender --background --python tools/blender/make_a1_prototype_pack.py -- --only arms
```

Generated A1 prototype asset ids:

- `wpn_a1_compact_rifle_01`
- `wpn_a1_modern_rifle_01`
- `wpn_a1_compact_sidearm_01`
- `chr_a1_stylized_operator_01`
- `chr_a1_fp_arms_01`

The A1 pack writes `.blend` sources under `assets/source/blender`, `.glb` exports under `assets/export/gltf`, metadata JSON next to each export, and `assets/export/gltf/a1_prototype_pack_manifest.json`.

## Generate A2 Visual Pack

From the Nemisis repository root:

```powershell
blender --background --python tools/blender/make_a2_visual_pack.py
```

If Blender is not in PATH:

```powershell
& "F:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --python tools/blender/make_a2_visual_pack.py
```

Generate one A2 group or asset:

```powershell
blender --background --python tools/blender/make_a2_visual_pack.py -- --only weapons
blender --background --python tools/blender/make_a2_visual_pack.py -- --only characters
blender --background --python tools/blender/make_a2_visual_pack.py -- --only map
blender --background --python tools/blender/make_a2_visual_pack.py -- --only wpn_a2_blackout_carbine_01
```

Generated A2 visual asset ids:

- `wpn_a2_blackout_carbine_01`
- `wpn_a2_modular_rifle_01`
- `wpn_a2_striker_sidearm_01`
- `chr_a2_pilot_operator_01`
- `map_a2_wallrun_panel_01`
- `map_a2_slide_ramp_01`
- `map_a2_cover_crate_01`
- `prop_a2_range_hero_01`

The A2 pack writes `.blend` sources under `assets/source/blender/a2_visual_pack`, flat `.glb` exports and metadata under `assets/generated/a2_visual_pack`, and `assets/generated/a2_visual_pack/manifest.json`.

Render the A2 preview sheet:

```powershell
blender --background --python tools/blender/render_a2_preview.py
```

If Blender is not in PATH:

```powershell
& "F:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --python tools/blender/render_a2_preview.py
```

The preview script imports the generated A2 GLBs, arranges them in a simple studio scene, and writes `assets/generated/a2_visual_pack/a2_visual_pack_preview.png` plus a reusable preview `.blend` file.

## Codex Agent Handoff

- `CODEX_ASSET_AGENT.md` describes the expected agent workflow.
- `asset_job_template.json` is a reusable job shape for single-asset tasks.
- `configs/assets/nemisis_assets.json` is the runtime contract for asset ids that Nemisis expects.
