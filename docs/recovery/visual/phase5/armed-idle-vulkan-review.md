# Integrated Armed Idle Vulkan candidate — 2026-10-19

The separate Vulkan game EXE is built and frozen. It has not been launched or
visually accepted. The earlier GUI pause remains in force until a fresh grant.

- Game runtime: `7d6b9a57a3091bfb014d455b2aec4811d482e8d4`, Draft game #37.
- Public engine runtime: `383505fbf283c14ded424dc7a1d6c1be47ee09e9`, Draft engine #26.
- Complete graphical game target: 310 build steps, Release GCC13.2, native UI,
  Jolt, Project and hand IK enabled. Full headless game: 212 steps, 48/48 CPU.
- EXE SHA256: `e711b209190b3018b8f64aeff1e9e45ad682da2a9216b934e94f69b75103f874`.
- [Exact paths/content hashes](armed-material-review-manifest.json). All three
  cooked Ready GLBs are byte-identical between the headless and Vulkan builds.
- R4 direct light is included; R5 sky reflections and the Shotgun candidate are
  separate work and are absent from this build.

Launch from the recorded game worktree after explicit permission:

```powershell
& C:/Users/Administrator/.codex/tmp/nemisis-phase5-armed-material-vulkan/nemisis_game.exe --vulkan --project-rifle-ready
```

Add `--project-fps-look-down` for the initial body view. For matched comparisons
change only `--material-textures` or `--directional-shadows`; `--physics-props`
adds the six local crates. Keep spawn/view, 1280x720, 74-degree hip FOV, lighting,
settings and content fixed. Record original images and backend statistics.

F01–F03 criteria: right firing fingers/wrist readable around the actual pistol
grip/trigger, left support under the fore-end, plausible shoulder/stock and
body/arms continuity, weapon below/right and a usable central aiming region.
No new sight alignment, reload, directional armed animation or other-weapon
acceptance follows from this Idle candidate. Shadow/texture quality, moving
casters, crate interaction, validation messages, resize and timing require actual
device evidence. The original failed Vulkan captures remain unchanged.
