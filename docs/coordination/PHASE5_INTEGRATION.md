# Phase 5 integration

This is the active coordination record. Phase 4 CPU results remain historical
evidence; they do not establish physical handedness or visual acceptance.

## Verified starting revisions

GitHub and local worktrees were checked on 2026-10-18. Original dirty checkouts
and the separate UI worktrees were not changed.

| Repository / work | Revision / state at start |
| --- | --- |
| Nemisis main | f15bc0e07281b4beab6899f4688fd703277d4850; #23 merged |
| Nemisis #25, #26, #27 | e2b0488, 571d264, 2264e27; open Drafts, MSVC/Clang green |
| Nemisis #28 | faf02ea5f22b67f0ed3e4b27718e1c426e800a33; open Draft |
| NovaCore main | 38e718bad65001879b482f8039bff338855e5f0b; #12/#13 merged |
| NovaCore #14 | fb33cb5502e6a4ea59ddc6183329529f25c0c279; open Draft, MSVC/Clang green, conflicts against main |
| Combined engine candidate | 5518818a6b89f20b0ad2ddb3c959116786dd9189; local merge of current main and #14 |
| Combined game candidate | 143efac3693f2743e638f9f130950c61080dbf1a; local merge of #27, current main and #28 |

The combined engine retains both CameraMath and VulkanUiRenderer includes and
both progress records. No renderer or UI implementation was replaced to resolve
the merge. These are candidate branches, not merges to GitHub main.

## Ownership and sequence

| Stream | Isolated branch / worktree | First deliverable |
| --- | --- | --- |
| A: First person | recovery/phase5-physical-grips; phase5/first-person/Nemisis | FP-1 physical right-firing / left-support fix, actual mesh regressions |
| B: Physics | recovery/phase5-physics-adoption; phase5/physics/Novacore-Engine | PH-1 audit/adoption gate, then opt-in Jolt body/query slice |
| C: Rendering | recovery/phase5-shadow-maps; renderer's existing isolated engine worktree | R2 directional depth maps; R3 texture sampling follows separately |
| Main | recovery/phase5-coordination; phase3/integration/Nemisis | Review, shared-file integration, exact pins and UI handoff |

At most two agents make large implementation changes at once. Initially A and C
write while B audits. Once the physics gate is ready, A and B write while C is
limited to isolated shadow maths/tests/shaders. Heavy local builds are scheduled
serially because RAM and disk are constrained. No shared checkout or branch is
mutated concurrently.

Main coordinates GameApp, root CMake, engine pins and the UI merge. Renderer owns
its backend/API changes only after that base is delivered. Physics has permission
for one coordinated CMake include plus its own helper/package configuration.
The UI chat retains UI documents, components, appearance and screen migration.

## Integration gates

1. FP-1 distinguishes actual weighted anatomical bones, physical pistol grip /
   handguard, socket labels and screen placement. Audit both default legacy and
   opt-in Project Ready. Screen-right X and small IK residual alone are insufficient.
2. Preserve chr_project_male1 in Project presentation. Correct each weapon once
   at the authoring/cook boundary; never mirror or apply runtime axis repairs.
3. Physics retains the existing KCC/replay path. Static collision comes from the
   existing PhysicsWorld data, with no separately authored collision scene. Dynamic
   props initially use explicit bounded interaction and cannot add player velocity.
4. R2 resources belong to the engine and the existing frame fence/swapchain
   lifecycle. The UI adapter keeps its private descriptor/pipeline state and final
   overlay order. Project FPS meshes require explicit shadow-caster decisions;
   their World layer does not make them an external character.
5. Validate each exact engine SHA against the game through public headers/targets
   before moving a pin. Test ordinary headless and opt-in UI/physics combinations.
   Record MSVC/Clang results and Vulkan compilation separately from execution.
6. Incomplete features remain opt-in/Draft. No automatic merges or replacement
   of existing defaults solely because CPU tests passed.

## Visual and performance evidence

On 2026-10-19 the user explicitly granted the exact frozen Phase 5 Vulkan build
`c04c78b` / engine `cca45a41`. Ready hipfire was inspected and **failed** framing
and grip readability; see [original evidence](../recovery/visual/phase5/README.md).
The subsequent look-down capture includes concurrent user input. GUI automation
is now paused while the user inspects the open window. Shadow/crate comparisons
remain unrun. CPU intersections and contact residuals are insufficient visual
acceptance. All six hosted #33 integration jobs pass independently of that result.

Physical Escape ended the previous GUI session. Phase 5 grants implementation,
builds and PR publication; GUI work requires an explicit grant and respects a
subsequent user pause. The paragraph above supersedes earlier pre-review records.

Phase 4 saved only phase4/visual/baseline_hip.jpg. Its manifest records the
accidental post-stop R0 launch; no R0/R1 after image exists. It is qualitative
baseline evidence, not a matched image/performance comparison.

Next authorized capture matrix: exact EXE/engine/content hashes, reset spawn and
tick, 1280x720, FOV 55/74/105, fixed light; M4/FR17 Hip/ADS/look-down, movement,
recoil/reload/switch; static and moving shadow casters, sun/horizon, UI transition
and resize. Record device, frame samples and validation messages. Missing gates
remain open. Historical screenshots and external art references are not runtime
proof. The downloaded reference art remains outside shipped assets.

See [UI_HANDOFF.md](UI_HANDOFF.md) for the explicit UI-chat contract.
