# Source Assets

Original editable assets belong here.

Expected layout:

```text
source/
  blender/
    characters/
    environments/
    props/
    weapons/
```

Current dev primitive `.blend` files were generated with Blender 5.1.2. Regenerate them with:

```powershell
.\tools\blender\run_make_dev_primitives.ps1 -Only all
```

The generator writes source `.blend` files for `prop_target_dummy_01`, `chr_player_capsule_proxy_01`, `chr_dev_arms_a`, `chr_dev_soldier_a`, `wpn_ar_01`, `wpn_smg_01`, `wpn_sidearm_01`, and `env_test_arena_kit_01`.

See `docs/16_ASSET_PRODUCTION_PLAN.md` and `assets/briefs/BLENDER_AGENT_BRIEFS.md` for the first production briefs.
