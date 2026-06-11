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
    canvas.crosshair(640.0F, 360.0F, 6.0F, 12.0F, nemisis::ui::palette::TextPrimary);

    expect(canvas.commandCount() == 4, "canvas records semantic commands");
    expect(canvas.frameDesc().width == 1920.0F, "canvas stores frame width");

    novacore::render::RenderFrameInfo frame{};
    canvas.appendToRenderFrame(frame);
    expect(frame.debugRects.size() == 3, "rounded rect and progress bar flush to debug rects");
    expect(frame.debugTexts.size() == 1, "text flushes to debug text");
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

} // namespace

int main() {
    testCanvasRecordsAndFlushesCommands();
    testImmediateHelpersAppendToFrame();

    if (failures > 0) {
        std::cerr << failures << " UI canvas test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis UI canvas tests passed\n";
    return EXIT_SUCCESS;
}
