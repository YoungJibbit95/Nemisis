# UI roadmap

The first reviewable milestone is UI-0 plus the UI-1 core component showcase.
No shipping-menu replacement or automatic merge is included in this PR.

| Phase | Current state | Next acceptance step |
| --- | --- | --- |
| UI-0 Technology | RmlUi 6.1 / FreeType adopted; native Vulkan geometry, fonts, gradients, glow and input work at 720p | Final review plus live resize / 1080p / 1440p acceptance |
| UI-1 Components | Core showcase: button states, toggles, slider, dropdown, radio, tabs, panels/cards, progress, modal, toast and key hints | Extend reusable screen components, animation/contrast acceptance |
| UI-2 Main menu | Existing GameMenu preserved; Showcase can launch actual Dev Range or return to legacy menu | Migrate visual layer while retaining state ownership |
| UI-3 Settings | Existing settings/persistence preserved | Native controls with draft/apply/cancel/reset semantics |
| UI-4 HUD | Existing gameplay HUD and debug overlay preserved | Migrate true health/ammo/feedback, separate default telemetry |
| UI-5 Class editor | Existing loadout/attachment data preserved | Native selection and actual buildWeaponWithAttachments stats |
| UI-6 Weapon preview | Not started | Agree isolated preview target API; use real assets/sockets |
| UI-7 Production | Not started | Effects capability, full input/device matrix, GPU timings and long runs |

## Exact revisions and review boundary

Game implementation: 2b16367870ae63dc360bfac19046c1f37460c6a9,
draft https://github.com/YoungJibbit95/Nemisis/pull/28. The branch also integrates
main f15bc0e after the parallel Ready PR landed; its content is preserved.


- Normal game NovaCore pin is 3d673b7345d295588367b466a8877e84dee112a9 (R1).
- Optional UI build: f0f7c54852169e6210fa23ba1221edce448f5ece, recorded separately
  in cmake/novacore-ui-revision.txt. Engine draft:
  https://github.com/YoungJibbit95/Novacore-Engine/pull/14.
- RmlUi, FreeType and font source commits and licenses are recorded in
  assets/ui/THIRD_PARTY_NOTICES.txt and cmake/RmlUi.cmake.
- Native UI is opt-in: -DNEMISIS_ENABLE_RMLUI=ON, then --ui-showcase.
  --ui-size=1280x720 / 1920x1080 / 2560x1440 selects a reproducible preview size.

## Validation performed

- Complete local GCC 13.2 Release game build without SDL/Vulkan: 34/34 CPU cases.
- Complete local SDL3/Vulkan game build including real RmlUi and FreeType.
- Native document test passes: layout bounds/actual font advances/atlas at all
  three resolutions; actual click, disabled control, checkbox, slider keyboard
  adjustment, tabs, dropdown selection, modal Tab trap, controller A/B and dropdown focus routing;
  120 warm idle frames retain stable atlas bytes and command count; no RmlUi
  style/font/render warnings.
- Separate engine complete Vulkan build and shaders: 5/5 CTest suites.
- Live 1280x720 Vulkan window inspected using the newly authorized Computer Use:
  modern text, rounded panels, linear gradient, font glow, hover/focus, actual
  button counter, modal open/Escape, dropdown High selection, typography including
  German umlauts/ß. Launch Dev Range reaches real gameplay; Escape returns to
  the existing menu, preserving mouse-capture ownership. Live Vulkan drawn UI
  differs clearly from the untouched legacy debug UI.
- Live last-frame CPU sample before transition: update/layout 0.2369 ms,
  draw extraction 0.1502 ms, 76 requested draws, 1,966,080 atlas bytes. One sample
  is not a benchmark. GPU pass cost has not been measured.
- The user stopped Computer Use with physical Escape. No further GUI input or
  graphical test launches followed. Final minor spacing adjustment, live resize,
  1080p/1440p visual acceptance and the three registered GPU smoke cases remain
  unverified. CPU geometry tests at those resolutions are not graphics proof.
- [720p typography evidence](evidence/typography-720p.png) is an actual captured
  Vulkan window, not a mockup.

## Status

**Functionally Complete:** the implemented showcase controls and real Dev Range /
menu transitions; this does not mean later menu/HUD/editor phases are complete.

**Visually Accepted:** provisional 720p inspection only. Final spacing, larger
resolutions, resize and a complete device/contrast review remain open.

**Production Ready:** no. Backdrop blur, optional advanced render interfaces,
GPU timing, image loading, full controller hardware tests and production screen
migration are pending. Blur is visibly labelled as a transparent fallback.

The UI pin combines the adapter with the merged R0 camera projection and R1
instance normals. The older adapter pin lacks camera roll and cannot build
the integrated Dev Range. Engine Vulkan build, seven CPU suites and all
17 shader modules pass for this combined revision; game CI validates both pins.
