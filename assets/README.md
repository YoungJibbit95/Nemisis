# Assets

Assets enter the project through the Blender-to-glTF production path described in `docs/16_ASSET_PRODUCTION_PLAN.md`.

This directory intentionally starts empty except for documentation. Do not add unclear-license marketplace assets here.

Current asset planning files:

- `briefs/BLENDER_AGENT_BRIEFS.md`: ready-to-hand-off briefs for a Blender-capable agent.
- `source/README.md`: editable source asset layout.
- `export/README.md`: runtime export layout.

Blender is not currently installed in the local environment, so this repo contains asset plans, briefs, and prepared automation, but no generated `.blend` or `.glb` assets yet. Once Blender is available through CLI, Python automation, or an MCP/plugin, the first generated files should be dev primitives: target dummy, floor grid, arena kit blockouts, and weapon blockouts.

Prepared automation:

- `tools/blender/make_dev_primitives.py`: Blender-only script for generating the first target dummy, AR blockout, and movement arena kit source/export files.







