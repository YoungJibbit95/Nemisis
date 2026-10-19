# Nemisis UI design system

## UI-0 technology decision

Use **RmlUi 6.1 + FreeType 2.13.3**, both pinned to immutable revisions in
`cmake/RmlUi.cmake`. Use Space Grotesk Regular/Bold (one family, OFL 1.1).
Notices and upstream licenses ship in `assets/ui/`. The fonts are unmodified.

| Criterion | RmlUi + NovaCore adapter | Existing UiCanvas |
| --- | --- | --- |
| Layout / animation | Existing RCSS layout, flex, transitions, dp scaling | Handwritten fixed 1280x720 layout and widget animation would expand game code |
| Fonts | FreeType rasterization, glyph advances, kerning, UTF-8 | Current measureText counts fixed bitmap cells; replacement required |
| Input | Native controls, hit testing, Tab focus, modal documents | Existing GameMenu actions/controller supported; keep during migration |
| Vulkan / SDL3 | Public CPU draw packet; engine owns all Vulkan objects | Current debug rectangles, lines and bitmap cells |
| Effects | Native rounded borders, vertex gradients, font glow; advanced effects need additional backend capabilities | All advanced effects need new primitives |
| Dependencies | RmlUi MIT; FreeType FTL; one OFL font family | Smaller initial dependency graph, larger maintenance surface |
| Cost / risk | Separate small engine integration; cached atlas textures and per-frame buffers | Would require a bespoke text/widget/layout framework |

Upstream references: [RmlUi](https://github.com/mikke89/RmlUi/tree/6.1),
[render interface](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/interfaces/render.html),
[Space Grotesk and license](https://github.com/floriankarsten/space-grotesk),
[FreeType license](https://freetype.org/license.html).

This spike intentionally implements the basic RmlUi render interface. No RmlUi
sample Vulkan backend, independent device/swapchain or SDL renderer is used.
Native linear vertex gradients and font-atlas glow work on that interface.
Filters, backdrop blur, arbitrary gradient shaders, offscreen layers, stencil
clip masks and image-file loading are not implemented by this adapter yet.
The visible panel explicitly labels the transparent blur fallback. Avoid RCSS
box-shadow/filter/mask/opacity layers that require unsupported optional methods.
Rounded panel geometry works; scroll clipping is rectangular.

## Existing UI audit

- GameMenu owns screen/tab/selection/loading state. GameApp retains settings,
  loadout, persistence and game transitions; UiCanvas owns frame commands only.
- Window polls SDL once into InputSnapshot. InputActionMap adds edges/device
  information. RmlUi consumes these snapshots/actions, not SDL events directly.
- Current menu/HUD coordinates and pointer mapping are 1280x720. UiCanvas appends
  debug primitives to RenderFrameInfo. NovaCore draws rectangles, then text,
  then lines, which cannot preserve arbitrary widget painter ordering.
- The new `RenderFrameInfo::ui` keeps RmlUi painter order in one ordered draw list
  after legacy overlays, inside the existing Vulkan render pass.
- Existing UiCanvas uses a 5x7 bitmap font and fixed character width. It remains
  a legacy bridge for unconverted screens, never the showcase typography path.
- New layout uses actual drawable size plus dp ratio based on 720p, FreeType
  metrics and flex panels. Pointer coordinates share Window's current size
  space; the backend converts scissors to swapchain pixels. Mixed-DPI/monitor
  transitions still need separate hardware acceptance.
- Mouse capture stays with GameApp's existing gameplay transition. Showcase
  runs over MainMenu, so fixed-tick gameplay and gameplay edge capture remain
  inactive. Launch Dev Range closes the document before capture is restored.
- Settings and loadouts retain their current GameSettings,
  UserSettingsPersistence, WeaponLoadout and AttachmentRegistry owners. Showcase
  controls are explicitly local specimens and never persist pretend settings.
- Existing UI canvas/menu and settings/loadout tests remain unchanged. The new
  native tests additionally exercise the actual RmlUi document and input path.

## Tokens and components

`assets/ui/theme.rcss` is the shared theme source. Palette: background #080F17,
surface #101C2A, elevated #192A38, cyan #16D9EB, blue #4D7DFF, highlight #8CF7FF,
text #F1F7FA, secondary text #93A7B8, success #35E3A1, warning #FFB65C,
danger #FF5B72. All geometry/font sizes use dp; regular/bold are real font faces.

Buttons: primary, secondary, ghost, navigation; normal/hover/focus/pressed/
selected/disabled. RmlUi button tags are registered as native input button
controls to preserve disabled and focus semantics. Controls: checkbox/toggle,
range slider, dropdown, radio group, tabs, cards, progress specimen, modal,
notification and keyboard hints. Settings changes are local previews. Focus
and hover transitions last 100–150 ms. Reduced Motion removes movement and
transitions immediately. No animation queue or continuous idle animation.

## Renderer ownership and cost

The game sends premultiplied RGBA8 atlas sources and indexed, transformed
vertices through a dependency-light public NovaCore packet. The engine owns
buffers, images, views, descriptors, sampler and pipeline. Buffers are reused
per in-flight slot and updated only after that slot's fence. Each slot retains
texture sources until completion. Expired textures are retired only after all
slot references disappear. Swapchain recreation waits for device idle and
rebuilds the adapter with the new render pass. No raw Vulkan API leaks to game.

New atlas textures use an immediate upload on the engine's existing graphics
queue and wait for that queue. This is a deliberate spike limitation, not a
performance improvement claim. Optimize only after measuring actual hitches.
Packets reject invalid texture sizes, indices, non-finite vertices and clips.
CPU update/layout, draw extraction, draw count, vertex count and atlas bytes are
available from UiShowcase::stats and logged when the showcase closes. GPU pass
time, dedicated layout time and long-session memory measurements remain open.

## Reproduce

Use the exact engine adapter revision recorded in UI_ROADMAP.md; the ordinary
game pin stays unchanged until the separate engine PR is reviewed.

```powershell
cmake -S . -B build/ui-preview -G Ninja -DCMAKE_BUILD_TYPE=Release `
  -DNOVACORE_ENGINE_ROOT=../Novacore-Engine -DNEMISIS_ENABLE_RMLUI=ON `
  -DNOVACORE_ENABLE_SDL3=ON -DNOVACORE_ENABLE_VULKAN=ON
cmake --build build/ui-preview --parallel 2
ctest --test-dir build/ui-preview -R nemisis_ui_showcase --output-on-failure
.\build\ui-preview\nemisis_game.exe --ui-showcase
```

Run from the repository root when runtime copying is disabled. Standard runtime
staging copies the UI/font/license tree beside the executable. An SDK with glslc
and SDL3 are required for graphics. `--ui-showcase` rejects a null/SDL renderer;
`--ui-showcase --smoke-test` is a bounded five-frame live Vulkan launch.

Acceptance: inspect Components, Typography and Input lab; hover/focus/press;
drag/adjust slider; checkbox/dropdown/radio; modal focus trap and Escape/B;
launch Dev Range and return with Escape; normal close. Inspect 1280x720,
1920x1080 and 2560x1440. Automated layout tests alone are not visual acceptance.
