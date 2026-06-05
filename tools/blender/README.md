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

## Codex Agent Handoff

- `CODEX_ASSET_AGENT.md` describes the expected agent workflow.
- `asset_job_template.json` is a reusable job shape for single-asset tasks.
- `configs/assets/nemisis_assets.json` is the runtime contract for asset ids that Nemisis expects.
