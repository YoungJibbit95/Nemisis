# Phase 5 native Vulkan evidence — 2026-10-19

Result: **Ready hipfire presentation fails**. The receiver sits too close to the
eye, fills the right side and hides the firing hand. The support fingers are not
wrapped convincingly. An actual Armed Idle with corrected clavicles, palms and
fingers is required. These images are evidence of the defect, not an accepted
after state or a matched performance comparison.

The three PNGs are original native window captures, without crop, recoloring or
retouching. Window dimensions are 1282x752 including decoration (1280x720 client).
The explicit fresh user grant covered this frozen build. Automation was then
paused because the user is inspecting the open look-down window.

| Capture | Conditions / limitation |
| --- | --- |
| [Initial hipfire](ready-hip-initial.png) | `--project-rifle-ready`; default 74-degree hip FOV, shadows off, no physics fixture |
| [Hipfire after reset](ready-hip-reset.png) | Same executable and content; reset input, qualitative framing evidence |
| [Uncontrolled look-down](ready-lookdown-uncontrolled.png) | `--project-rifle-ready --project-fps-look-down`; simultaneous user movement, x≈-19.6m / airborne / grav-boots; not a controlled body-continuity result |

Exact game runtime: `c04c78bf6d95d30e69a611619ad8b30b4430d0da`.
Exact engine: `cca45a41aded8b13171943553e27f85071c76ed3`.
EXE SHA256: `5be1c86eb12f55055e247ed312bfb90770e92a360e16bb2e870b3b4d52c8fc78`.
Ready GLB SHA256: `f59c8151c3950778ae457f748b2bb4341ff62d6b623597b5c1c2ea716cd5cc5e`.
See [manifest.json](manifest.json) for original capture/library hashes.

The hipfire run shut down normally: 6238 body and arm samples/skins/accepted GPU
updates, one logical pose owner, zero invalid or unreachable hand IK samples.
This confirms execution/update plumbing, not anatomical quality or GPU timing.
Background builds and uncontrolled interaction preclude performance conclusions.
Shadow-on, crate interaction, ADS/reload, resize and Vulkan validation-layer
acceptance remain open. Future matched captures must hold spawn, view, lighting,
settings and content constant and use a newly reviewed candidate build.
