# Game development team

This Codex setup serves Nemisis and its NovaCore engine dependency. Start Codex at the root of the configured two-repository workspace. A standalone Nemisis checkout can load the same profiles when opened as a trusted project. Read the project rules in the checkout that owns the change. Game mechanics, tuning, content and presentation belong to Nemisis. Reusable runtime capabilities belong to NovaCore.

The engine-neutral workflows in `.agents/skills/` are vendored from [Everything Game Dev Code at a483ba7](https://github.com/MRCalderon3D/everything-game-dev-code/tree/a483ba7850f0081f0d47247e3bf67f4a3d4709c8/skills). The [upstream MIT license](LICENSE-everything-game-dev-code.txt) accompanies these files. The installation consists of the selected Markdown skills. Each folder name matches the corresponding upstream skill.

The lead uses `orchestration-patterns` to give each work item one owner, a repository, a handoff and an observable acceptance criterion. For example, the rigging owner supplies grip sockets and a reproducible pose. The animation owner checks the pose in the game camera. The gameplay and networking owners verify that the same action produces the expected server result. The individual profiles under `.codex/agents/` name the workflows for each specialty.

| Area | Lead | Specialists |
| --- | --- | --- |
| Gameplay and animation | `gameplay_animation` | `technical_rigging`, `gunplay_targets` |
| Physics and networking | `simulation_netcode` | `traversal` |
| Rendering and world presentation | `world_presentation` | `renderer_runtime`, `environment_art`, `hud_feedback` |
| Cross-team verification | `performance_qa` | Works with the change owner |

Submit focused changes as pull requests targeting the owning repository's `main`. Record the exact engine revision for game integration and include reproducible checks and a short playtest for player-facing changes. The project owner playtests and merges.
