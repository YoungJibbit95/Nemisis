# Asset Import Readiness

Generated: 2026-06-13

Scan command:

```powershell
python tools\assets\asset_readiness_check.py --repo-root . --output assets\processed\readiness\asset_readiness_scan.json
```

## Scope

- Checked `assets/source/blender`, `assets/export/gltf`, `assets/generated`, `assets/project_assets`, `configs/assets/nemisis_assets.json`, and existing asset tooling.
- The imported Project GLB drop is now repo-local under `assets/project_assets`; `F:\Coding\Project\Assets` is only a staging location for newly dropped files.
- Original `.blend` assets were not rewritten. Sidecar metadata was added next to the imported Project GLBs and skybox, and the audit output is `assets/processed/readiness/asset_readiness_scan.json`.
- This scan did not request Blender validation. Run the same command with `--with-blender` when Blender transform checks are needed.

## Summary

- Catalog entries: 38 total, 36 runtime mesh/scene GLBs plus 2 material IDs.
- GLB inventory: 36 repo-local runtime GLBs under `assets/`, including the imported Project character/weapons plus `skybox1.glb`; 0 orphan repo GLBs, 0 uncataloged runtime `.blend` sources.
- Format: every scanned cataloged GLB is binary glTF 2.0 with an embedded BIN chunk.
- Texture/material references: no scanned GLB uses external buffer or image URIs. Imported Project GLBs carry embedded image data.
- Blender source scan: not requested in this pass. The previous Blender-enabled audit opened 21 cataloged `.blend` files headless with meter scale and no unapplied mesh transforms.
- Root/pivot note: A1/A2 sources include root empties named after the asset at origin. Older A0 dev primitives do not, but their sockets/collision nodes are present and transforms are applied.

## Game-Ready Now

These assets are ready for visual/runtime import by ID. They have source `.blend`, GLB export, metadata, sockets, dimensions, origin notes, `scale_meters: true`, `runtime_up_axis: "Y"`, `gameplay_forward_axis: "+Z"`, `license: "original_project_asset"`, and `external_assets: false`.

| Import ID | Use | Key sockets/collision |
| --- | --- | --- |
| `wpn_a1_compact_rifle_01` | A1 compact rifle fallback | muzzle, grip L/R, eject, vfx; no collision |
| `wpn_a1_modern_rifle_01` | A1 rifle fallback | muzzle, grip L/R, eject, vfx; no collision |
| `wpn_a1_compact_sidearm_01` | A1 sidearm fallback | muzzle, grip L/R, eject, vfx; no collision |
| `chr_a1_stylized_operator_01` | A1 third-person operator | root, camera, weapon root, hands, head, backpack, vfx; `col_chr_a1_stylized_operator_01_capsule` |
| `chr_a1_fp_arms_01` | A1 first-person arms | camera, weapon root, hands, vfx; no collision |
| `wpn_a2_blackout_carbine_01` | A2 compact carbine | muzzle, grip L/R, optic, mag, eject, vfx; no collision |
| `wpn_a2_modular_rifle_01` | A2 primary rifle candidate | muzzle, grip L/R, optic, underbarrel, mag, eject, vfx; no collision |
| `wpn_a2_striker_sidearm_01` | A2 sidearm candidate | muzzle, grip L/R, optic, rail, eject, vfx; no collision |
| `chr_a2_pilot_operator_01` | A2 third-person operator | root, camera, weapon root, hands, head, backpack, vfx; `col_chr_a2_pilot_operator_01_capsule` |

Recommended first import IDs for runtime smoke tests:

- Primary first-person/world weapon: `wpn_project_rifle_m4a1`
- Compact weapon variant: `wpn_project_smg_fr17`
- Sidearm: `wpn_project_sidearm_glock19`
- Third-person target actor: `chr_project_male1`
- First-person arms fallback: `chr_a1_fp_arms_01`

## Runtime-Active Project Assets

These Project GLBs have been moved into `assets/project_assets`, cataloged, bound by `DevAssetBindings`, registered as renderer mesh resources, and submitted by the Dev Range render scene. They are not yet full production-ready imports because the source GLBs do not contain authored Nemisis `socket_*` empties, exported `col_` proxies, or filename stems matching their runtime IDs.

| Import ID | File | Ingame use |
| --- | --- | --- |
| `chr_project_male1` | `assets/project_assets/character_male1.glb` | Target-lane actor fallback/showcase character and local first-person body presentation |
| `wpn_project_rifle_m4a1` | `assets/project_assets/weapon_rifle_m4a1.glb` | Default first-person rifle, direct-switch AR, pickup rack |
| `wpn_project_rifle_afr120` | `assets/project_assets/weapon_rifle_afr120.glb` | Showcase rack rifle |
| `wpn_project_rifle_ncar` | `assets/project_assets/weapon_rifle_ncar.glb` | Shotgun-slot first-person placeholder and pickup rack |
| `wpn_project_smg_fr17` | `assets/project_assets/weapon_smg_fr17.glb` | SMG first-person placeholder, direct-switch SMG, pickup rack |
| `wpn_project_sidearm_glock19` | `assets/project_assets/weapon_sidearm_glock19.glb` | Sidearm first-person placeholder, direct-switch sidearm, pickup rack |
| `wpn_project_sidearm_p320` | `assets/project_assets/weapon_sidearm_p320.glb` | Showcase rack sidearm |
| `env_project_skybox1` | `assets/project_assets/skybox1.glb` | First-pass Dev Range sky environment mesh |

## Ready With Notes

The A0/dev primitive set is importable for dev sandbox use. The main gap is older metadata: these entries are missing `origin`, `dimensions_m`, `target_dimensions_m`, and `external_assets`. Their GLBs are valid, sockets exist, and Blender source transforms are applied.

| Import ID | Status note |
| --- | --- |
| `prop_target_dummy_01` | Has hit/vfx sockets and `col_prop_target_dummy_01`; add rich metadata before relying on automated placement. |
| `wpn_ar_01` | Has muzzle/grip/eject/vfx sockets; add origin/dimensions metadata. |
| `wpn_smg_01` | Has muzzle/grip/eject/vfx sockets; add origin/dimensions metadata. |
| `wpn_sidearm_01` | Has muzzle/grip/eject/vfx sockets; add origin/dimensions metadata. |
| `env_test_arena_kit_01` | Has spawn/control sockets and per-piece `col_` meshes; add kit dimensions/origin metadata. |
| `chr_player_capsule_proxy_01` | Has root/camera/weapon/hit sockets and `col_chr_player_capsule_proxy_01`; add dimensions/origin metadata. |
| `chr_dev_arms_a` | Has camera/weapon/hand sockets; add dimensions/origin metadata. |
| `chr_dev_soldier_a` | Has root/camera/weapon/hand/head/vfx sockets and `col_chr_dev_soldier_a_capsule`; add dimensions/origin metadata. |

## Needs Manual Work

- `mat_wpn_ar_polymer_dark` and `mat_env_movement_accent` are cataloged, but their JSON sources under `assets/materials/...` do not exist. Either add those material definitions or remove/replace the catalog dependencies.
- Prototype-pack IDs are not game-ready as-is. They are useful for visual/greybox smoke tests, but their metadata and GLB nodes do not currently agree.
- `wpn_proto_smg_01`: metadata declares `socket_root`, `socket_grip_r`, and `socket_grip_l`, but the GLB contains `socket_grip`, `socket_mag`, and `socket_muzzle`; forward axis is `+X`.
- `chr_proto_humanoid_01`: declares `col_chr_proto_humanoid_01_capsule`, but no matching `col_` node is exported; forward axis is `+Y`.
- `map_wall_panel_01`, `map_floor_tile_01`, `map_cover_crate_01`, `map_ramp_01`, and `map_target_stand_01`: metadata declares sockets that are not present in the GLBs, uses `+Y` forward, and relies on simple runtime collision labels rather than exported `col_` proxies.
- A2 map/prop assets `map_a2_wallrun_panel_01`, `map_a2_slide_ramp_01`, `map_a2_cover_crate_01`, and `prop_a2_range_hero_01` are visually import-ready, but collision is only documented as `visual_only_use_*`. Add exported `col_<id>` proxies or explicit runtime collision metadata before using them for authoritative movement/physics.

Current runtime bridge:

- The Dev Range now provides temporary runtime collision proxies for visible A2 stage pieces in `GreyboxWorld`.
- Runtime proxy ids include `asset_a2_slide_ramp_collision`, `asset_a2_wallrun_panel_collision`, `asset_a2_cover_crate_collision`, `asset_a2_range_hero_collision`, and asset-stage plinth/backboard collision ids.
- These proxies make the current smoke-test scene playable, but they are not a replacement for authored `col_` nodes or collision metadata in production assets.
- The game asset catalog now resolves Project asset IDs from committed repo-local paths under `assets/project_assets`. Future asset-agent work should add authored sockets/collision/animation data instead of relying on runtime placement heuristics.
- Current Project preview IDs are `chr_project_male1`, `wpn_project_rifle_m4a1`, `wpn_project_rifle_afr120`, `wpn_project_rifle_ncar`, `wpn_project_smg_fr17`, `wpn_project_sidearm_glock19`, `wpn_project_sidearm_p320`, and `env_project_skybox1`.
- Project assets `chr_project_male1`, `wpn_project_rifle_m4a1`, `wpn_project_rifle_afr120`, `wpn_project_rifle_ncar`, `wpn_project_smg_fr17`, `wpn_project_sidearm_glock19`, and `wpn_project_sidearm_p320` are runtime-active but need a future non-destructive normalization pass with stable filename stems, `socket_muzzle`/grip sockets for weapons, root/camera/weapon sockets for the character, and optional `col_` proxies. `env_project_skybox1` is runtime-active as a temporary large GLB sky mesh until NovaCore gets a dedicated cubemap/skybox pass.

## Import Metadata Contract

Use catalog IDs exactly as import IDs. New production assets should ship metadata next to the GLB with this shape:

```json
{
  "id": "wpn_a2_modular_rifle_01",
  "source": "assets/source/blender/a2_visual_pack/wpn_a2_modular_rifle_01.blend",
  "export": "assets/generated/a2_visual_pack/wpn_a2_modular_rifle_01.glb",
  "category": "weapon",
  "scale_meters": true,
  "runtime_up_axis": "Y",
  "gameplay_forward_axis": "+Z",
  "blender_up_axis": "+Z",
  "blender_forward_axis": "+Y",
  "origin": "Origin at approximate right-hand grip/root; Blender forward axis is +Y.",
  "dimensions_m": [0.2359, 1.6385, 0.7062],
  "target_dimensions_m": [0.2, 1.63, 0.64],
  "sockets": ["socket_muzzle", "socket_grip_r", "socket_grip_l", "socket_eject", "socket_vfx"],
  "collision": "none",
  "lods": ["wpn_a2_modular_rifle_01_lod0", "wpn_a2_modular_rifle_01_lod1"],
  "license": "original_project_asset",
  "external_assets": false,
  "generated_by": "tools/blender/make_a2_visual_pack.py"
}
```

Socket expectations:

- Weapons: `socket_muzzle`, `socket_grip_r`, `socket_grip_l`, `socket_eject`, `socket_vfx`; add `socket_optic`, `socket_mag`, `socket_underbarrel`, or `socket_rail` where useful.
- First-person arms: `socket_camera`, `socket_weapon_root`, `socket_hand_r`, `socket_hand_l`, `socket_vfx`.
- Characters/operators: `socket_root`, `socket_camera`, `socket_weapon_root`, `socket_hand_r`, `socket_hand_l`, `socket_head`, optional `socket_backpack`, `socket_vfx`, and an exported `col_` capsule when used for hit/collision tests.
- Movement/map pieces: exported `col_` proxies for gameplay collision, or explicit runtime collision metadata; snap/movement sockets should be present in the GLB, not only in metadata.

## Reproduce

Run:

```powershell
python tools\assets\asset_readiness_check.py --repo-root . --output assets\processed\readiness\asset_readiness_scan.json
```

Add `--with-blender` for a full Blender-source transform audit, and use `--fail-on-error` if this should become a CI gate after the current manual-work items are resolved.
