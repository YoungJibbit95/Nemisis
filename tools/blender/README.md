# Blender Tools

These scripts are meant to run inside Blender, not plain Python.

Current local environment note:

- `blender` is not available in PATH.
- These scripts are committed now so the first asset automation step is ready as soon as Blender is installed or exposed through a Blender MCP/plugin.

## Generate Dev Primitives

From the Nemisis repository root:

```powershell
blender --background --python tools/blender/make_dev_primitives.py
```

Using the helper script:

```powershell
.\tools\blender\run_make_dev_primitives.ps1 -Only all
```

Generate one asset group:

```powershell
blender --background --python tools/blender/make_dev_primitives.py -- --only target
blender --background --python tools/blender/make_dev_primitives.py -- --only weapon
blender --background --python tools/blender/make_dev_primitives.py -- --only arena
```

The script writes:

- `.blend` source files under `assets/source/blender`.
- `.glb` exports under `assets/export/gltf`.
- Metadata JSON next to each export.

The generated assets are dev primitives only. They are meant to unblock gameplay testing, scale checks, sockets, and importer work before final art.

## Codex Agent Handoff

- `CODEX_ASSET_AGENT.md` describes the expected agent workflow.
- `asset_job_template.json` is a reusable job shape for single-asset tasks.
- `configs/assets/nemisis_assets.json` is the runtime contract for asset ids that Nemisis expects.
