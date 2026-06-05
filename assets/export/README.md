# Exported Assets

Runtime-ready exports belong here after validation.

Expected layout:

```text
export/
  gltf/
    characters/
    environments/
    props/
    weapons/
```

Do not place unclear-license marketplace files here. Every export should be traceable to an original source asset and metadata entry.

The current dev primitive exports were generated with Blender 5.1.2. Regenerate them with:

```powershell
.\tools\blender\run_make_dev_primitives.ps1 -Only all
```

Expected generated `.glb` ids:

- `prop_target_dummy_01`
- `chr_player_capsule_proxy_01`
- `chr_dev_arms_a`
- `chr_dev_soldier_a`
- `wpn_ar_01`
- `wpn_smg_01`
- `wpn_sidearm_01`
- `env_test_arena_kit_01`
