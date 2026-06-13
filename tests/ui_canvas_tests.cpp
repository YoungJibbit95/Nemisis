#include "nemisis/ui/UiCanvas.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

int failures = 0;

void expect(bool condition, std::string_view message) {
    if (condition) {
        return;
    }

    ++failures;
    std::cerr << "[fail] " << message << '\n';
}

void testCanvasRecordsAndFlushesCommands() {
    nemisis::ui::UiCanvas canvas;
    canvas.beginFrame(nemisis::ui::UiFrameDesc{1920.0F, 1080.0F, 1.0F});
    canvas.roundedRect({10.0F, 20.0F, 200.0F, 80.0F}, 8.0F, nemisis::ui::palette::Panel);
    canvas.text(24.0F, 40.0F, 2.0F, nemisis::ui::palette::TextPrimary, "NEMISIS");
    canvas.progressBar(
        {24.0F, 72.0F, 120.0F, 12.0F},
        0.5F,
        nemisis::ui::palette::PanelRaised,
        nemisis::ui::palette::Accent);
    canvas.image({180.0F, 72.0F, 64.0F, 32.0F}, "assets/ui/icons/loadout.svg", nemisis::ui::palette::Blueprint);
    canvas.crosshair(640.0F, 360.0F, 6.0F, 12.0F, nemisis::ui::palette::TextPrimary);

    expect(canvas.commandCount() == 5, "canvas records semantic commands");
    expect(canvas.frameDesc().width == 1920.0F, "canvas stores frame width");

    novacore::render::RenderFrameInfo frame{};
    canvas.appendToRenderFrame(frame);
    expect(frame.debugRects.size() == 7, "rounded rect segments, image placeholder, and progress bar flush to debug rects");
    expect(frame.debugTexts.size() == 2, "text and image asset label flush to debug text");
    expect(frame.debugLines.size() == 4, "crosshair flushes to four lines");
}

void testImmediateHelpersAppendToFrame() {
    novacore::render::RenderFrameInfo frame{};
    nemisis::ui::appendUiRect(frame, {1.0F, 2.0F, 3.0F, 4.0F}, nemisis::ui::palette::Accent);
    nemisis::ui::appendUiLine(frame, 0.0F, 0.0F, 10.0F, 10.0F, nemisis::ui::palette::Warning);
    nemisis::ui::appendUiText(frame, 8.0F, 9.0F, 2.0F, nemisis::ui::palette::TextSecondary, "HUD");

    expect(frame.debugRects.size() == 1, "appendUiRect writes one rect");
    expect(frame.debugLines.size() == 1, "appendUiLine writes one line");
    expect(frame.debugTexts.size() == 1, "appendUiText writes one text");
}

void testCanvasBackbonePrimitives() {
    nemisis::ui::UiCanvas canvas;
    canvas.beginFrame();
    canvas.panel(
        {16.0F, 20.0F, 320.0F, 160.0F},
        nemisis::ui::UiPanelStyle{
            nemisis::ui::palette::Panel,
            {0.16F, 0.32F, 0.36F, 0.80F},
            nemisis::ui::palette::Accent,
            10.0F,
            2.0F,
            true,
            true,
        });
    canvas.button({32.0F, 42.0F, 280.0F, 44.0F}, "Firing Range", "Ready", true);
    canvas.pill({32.0F, 104.0F, 108.0F, 24.0F}, "VULKAN", nemisis::ui::palette::AccentSoft, nemisis::ui::palette::TextPrimary);
    canvas.outlinedText(32.0F, 142.0F, 2.0F, nemisis::ui::palette::TextPrimary, "READABLE");

    const auto metrics = nemisis::ui::UiCanvas::measureText("READABLE", 2.0F);
    expect(metrics.width > 80.0F, "text metrics measure bitmap text width");
    expect(nemisis::ui::UiCanvas::fitTextScale("VERY LONG LOADOUT ROW LABEL", 80.0F, 2.0F) < 2.0F, "fitTextScale reduces oversized labels");

    novacore::render::RenderFrameInfo frame{};
    canvas.appendToRenderFrame(frame);
    expect(frame.debugRects.size() >= 6, "panel/button/pill backbone emits reusable rect primitives");
    expect(frame.debugLines.size() >= 5, "panel/button backbone emits border line primitives");
    expect(frame.debugTexts.size() >= 8, "shadowed and outlined text emit layered text primitives");
}

} // namespace

int main() {
    testCanvasRecordsAndFlushesCommands();
    testImmediateHelpersAppendToFrame();
    testCanvasBackbonePrimitives();

    if (failures > 0) {
        std::cerr << failures << " UI canvas test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis UI canvas tests passed\n";
    return EXIT_SUCCESS;
}
