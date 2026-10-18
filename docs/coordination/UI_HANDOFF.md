# UI handoff: Phase 5

## Integration candidate, 2026-10-19

The current game runtime candidate is `c04c78bf6d95d30e69a611619ad8b30b4430d0da`
on `recovery/phase5-dev-range-physics`; both engine pins use
`cca45a41aded8b13171943553e27f85071c76ed3` (engine Draft #19). This combines
the existing native UI with the proper RH camera, optional rigid bodies and
directional shadows. Engine standard/Jolt MSVC and Clang jobs are green; the
complete game build and48/48 CPU tests pass94.07s, and the full separate Vulkan
game builds. Hosted game integration CI follows. Earlier checkpoints below remain
historical. No Phase 5 GUI acceptance has occurred.

The actual new quality API is `RenderFrameInfo::directionalShadows`, configured
through `DevRenderTuning` and `configs/render/dev_range_render.json`. It provides
enabled, resolution, distance and bias values. It is currently a developer
config/CLI preview, not a persisted interactive GameSettings menu. Agree validated
UI actions with main before adding controls. Local FPS submissions have explicit
per-instance shadow exclusions; shared world resources remain eligible.

`--physics-props` is a local Dev Range fixture, not replicated match state or a
new source of authoritative damage/ammo. The existing gameplay state APIs below
retain ownership. R3 texture/sampler descriptors are subsequent work and remain
separate from UI atlas descriptors, slot fences and final overlay order. No
offscreen character/weapon preview or backdrop blur is promised by this change.

Keep the RmlUi 6.1 / FreeType 2.13.3 foundation from Nemisis #28. The separate
UI chat owns its visual design, components and screen migration. Main owns
integration with gameplay, physics and renderer work.

## Exact source boundary

- UI game foundation: faf02ea5f22b67f0ed3e4b27718e1c426e800a33, PR #28.
- UI engine adapter: fb33cb5502e6a4ea59ddc6183329529f25c0c279, PR #14.
- Engine main including accepted R0/R1:
  38e718bad65001879b482f8039bff338855e5f0b.
- Main's combined engine candidate:
  5518818a6b89f20b0ad2ddb3c959116786dd9189. Includes both renderer fixes and UI;
  original #14 conflicts were the include list and appended recovery records.
- Main's combined game candidate:
  143efac3693f2743e638f9f130950c61080dbf1a. Includes #27 and #28 without replacing
  either implementation. Final tested pins/results are recorded in PROGRESS.md.

These are review candidates. No PR is automatically merged and no source branch
owned by the UI chat is rewritten.

## Existing game-owned read APIs

Use these existing typed values at GameApp's current menu/HUD composition point;
do not read ECS or advance simulation from an RmlUi document/controller.

| UI need | Actual source / contract |
| --- | --- |
| Weapon name/class/base stats | activeAttachmentBuild_.effectiveWeapon, WeaponDefinition in weapons/WeaponTypes.hpp |
| Ammo, ADS, reload | DevSandbox::latestSample().weapon; WeaponRuntimeState::ammoInMagazine, adsAlpha, reloading, reloadProgress, reloadTimeRemaining |
| Health | latestSample().playerHealth; the same fixed-tick sample as ammo, not a new UI health owner |
| Movement | latestSample().movementMode, velocity, movementTech and tick |
| Loadout | WeaponLoadout and AttachmentRegistry in weapons/WeaponAttachments.hpp; buildWeaponWithAttachments is the existing effective-stat calculation |
| Settings | GameSettings in settings/GameSettings.hpp, existing adjustment functions and UserSettingsPersistence |
| Menu navigation | GameMenu::screen/tab/gameplayActive, showMainMenu/showDevRange/showLoadingScreen; UiShowcase returns ShowcaseAction for GameApp to apply |
| Available render settings | DevRenderTuning sky/lighting/FOV/near/far plus showWorldDebugLines; no invented quality setting may appear functional |
| Weapon/character IDs | GameAssetCatalog, DevAssetBindings::meshCatalog and existing mounts; Project character is chr_project_male1 |

`GameApp::onFrame` already passes these values to GameMenu::appendRenderCommands.
An RmlUi HUD/controller can receive immutable copies/const views at that same
composition point. Keep the fixed-tick sample tick with the HUD values. Mutations
must return validated user actions to GameApp and existing settings/loadout
functions. Never start another ammo/reload timer or persist showcase specimen
controls as real settings.

Preview selection can reuse catalog IDs. There is currently NO isolated 3D
preview render-target API. Request that small engine capability before adding a
weapon/character preview; do not construct a second device/swapchain or feed
preview transforms into the authoritative character.

## Renderer boundary to preserve

`RenderFrameInfo::ui` contains the ordered public UiDrawData packet. Positions
and clips share its width/height pixel space. Texture sources are immutable
premultiplied RGBA8 and kept alive with shared_ptr until all referencing slots
retire. NovaCore owns Vulkan objects, buffers, images, descriptors and samplers.

The engine prepares UI only after the current frame fence, then records it after
world/viewmodel and legacy overlays in the existing render pass. Resize follows
the existing idle/recreate boundary. R2 uses separate shadow layouts/resources;
it must preserve the UI's private pipeline layout and final overlay order.
The current first-use atlas upload waits for the graphics queue. This is an
explicit cost to measure, not an asynchronous streaming implementation.

Available UI effects include geometry, atlas text, vertex gradients and font
glow. Image-file loading, offscreen layers, stencil masks, advanced filters and
true backdrop blur are not supplied by the current bridge. Request capabilities
before depending on them; retain honest fallbacks. Native CPU layout tests at
720p/1080p/1440p do not establish visual resize/GPU acceptance.

## Art direction for the next UI change

- Give navigation, content and HUD distinct silhouettes: angled/chamfered panels,
  restrained hexagonal details, asymmetry, layered borders and vector contours.
- Separate glass, metal and carbon surfaces with edge light, subtle texture and
  controlled depth. Avoid making every panel the same translucent rounded card.
- Add subtle menu parallax/animated gradients and layered entrances; use real
  character/weapon backgrounds only when their presentation is stable. Keep
  target visibility and text contrast; support reduced motion.
- Animate navigation, hover/selection and HUD value changes with clear hierarchy.
  Avoid constant competing glows or unbounded effects.
- Keep UI changes on the existing foundation and agree shared backend/API changes
  with main before implementation. R2/R3 resource changes are active work.

## Message to copy to the separate UI chat

> Phase 5: Bitte RmlUi-/FreeType-Foundation aus #28/#14 beibehalten. Entwickle
> eigenständige angewinkelte/chamferte Shapes, geschichtete Glass-/Metal-/Carbon-
> Oberflächen, klarere Hierarchie und subtile animierte Hintergründe/Transitions.
> Competitive-Lesbarkeit und Reduced Motion erhalten. Hauptagent koordiniert
> die Kombination mit Sky/Normals und den kommenden Shadow-/Texture-Ressourcen.
> Nutze docs/coordination/UI_HANDOFF.md am veröffentlichten Integrations-Commit:
> dort stehen exakte SHAs, bestehende Ammo/Reload/Health/Loadout/Settings-Daten
> und Renderer-Lifetimes. UI liest State und gibt validierte Aktionen zurück;
> keine neue Gameplay-Simulation. Für 3D-Preview/Blur/Render-Targets erst API
> abstimmen. GameApp, CMake und VulkanBackend nur koordiniert ändern.

Publication/commenting is an explicit GitHub handoff, not proof that the separate
chat has read or accepted it. No direct cross-chat delivery is claimed.
