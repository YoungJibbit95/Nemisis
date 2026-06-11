#include "nemisis/ui/UiCanvas.hpp"

#include <algorithm>
#include <utility>

namespace nemisis::ui {

namespace {

[[nodiscard]] float clampedProgress(float value) {
    return std::clamp(value, 0.0F, 1.0F);
}

void appendCommandToFrame(const UiCommand& command, novacore::render::RenderFrameInfo& frame) {
    switch (command.kind) {
    case UiCommandKind::Rect:
    case UiCommandKind::RoundedRect:
        appendUiRect(frame, command.rect, command.color);
        break;
    case UiCommandKind::Line:
        appendUiLine(frame, command.x0, command.y0, command.x1, command.y1, command.color);
        break;
    case UiCommandKind::Text:
        appendUiText(frame, command.x0, command.y0, command.scale, command.color, command.text);
        break;
    case UiCommandKind::ProgressBar:
        appendUiRect(frame, command.rect, command.secondaryColor);
        appendUiRect(
            frame,
            UiRect{
                command.rect.x,
                command.rect.y,
                command.rect.width * clampedProgress(command.value),
                command.rect.height,
            },
            command.color);
        break;
    case UiCommandKind::Crosshair: {
        const float cx = command.x0;
        const float cy = command.y0;
        const float gap = command.radius;
        const float length = command.value;
        appendUiLine(frame, cx - gap - length, cy, cx - gap, cy, command.color);
        appendUiLine(frame, cx + gap, cy, cx + gap + length, cy, command.color);
        appendUiLine(frame, cx, cy - gap - length, cx, cy - gap, command.color);
        appendUiLine(frame, cx, cy + gap, cx, cy + gap + length, command.color);
        break;
    }
    }
}

} // namespace

void UiCanvas::beginFrame(UiFrameDesc desc) {
    frame_ = desc;
    clear();
}

void UiCanvas::clear() {
    commands_.clear();
}

void UiCanvas::rect(UiRect rect, UiColor color) {
    UiCommand command{};
    command.kind = UiCommandKind::Rect;
    command.rect = rect;
    command.color = color;
    commands_.push_back(std::move(command));
}

void UiCanvas::roundedRect(UiRect rect, float radius, UiColor color) {
    UiCommand command{};
    command.kind = UiCommandKind::RoundedRect;
    command.rect = rect;
    command.radius = std::max(0.0F, radius);
    command.color = color;
    commands_.push_back(std::move(command));
}

void UiCanvas::line(float x0, float y0, float x1, float y1, UiColor color) {
    UiCommand command{};
    command.kind = UiCommandKind::Line;
    command.x0 = x0;
    command.y0 = y0;
    command.x1 = x1;
    command.y1 = y1;
    command.color = color;
    commands_.push_back(std::move(command));
}

void UiCanvas::text(float x, float y, float scale, UiColor color, std::string text) {
    UiCommand command{};
    command.kind = UiCommandKind::Text;
    command.x0 = x;
    command.y0 = y;
    command.scale = std::max(0.1F, scale);
    command.color = color;
    command.text = std::move(text);
    commands_.push_back(std::move(command));
}

void UiCanvas::progressBar(UiRect rect, float value, UiColor background, UiColor foreground) {
    UiCommand command{};
    command.kind = UiCommandKind::ProgressBar;
    command.rect = rect;
    command.value = clampedProgress(value);
    command.secondaryColor = background;
    command.color = foreground;
    commands_.push_back(std::move(command));
}

void UiCanvas::crosshair(float centerX, float centerY, float gap, float length, UiColor color) {
    UiCommand command{};
    command.kind = UiCommandKind::Crosshair;
    command.x0 = centerX;
    command.y0 = centerY;
    command.radius = std::max(0.0F, gap);
    command.value = std::max(0.0F, length);
    command.color = color;
    commands_.push_back(std::move(command));
}

void UiCanvas::appendToRenderFrame(novacore::render::RenderFrameInfo& frame) const {
    for (const auto& command : commands_) {
        appendCommandToFrame(command, frame);
    }
}

const std::vector<UiCommand>& UiCanvas::commands() const {
    return commands_;
}

std::size_t UiCanvas::commandCount() const {
    return commands_.size();
}

UiFrameDesc UiCanvas::frameDesc() const {
    return frame_;
}

void appendUiRect(novacore::render::RenderFrameInfo& frame, UiRect rect, UiColor color) {
    frame.debugRects.push_back(novacore::render::DebugRect{
        rect.x,
        rect.y,
        rect.width,
        rect.height,
        color,
    });
}

void appendUiLine(
    novacore::render::RenderFrameInfo& frame,
    float x0,
    float y0,
    float x1,
    float y1,
    UiColor color) {
    frame.debugLines.push_back(novacore::render::DebugLine{x0, y0, x1, y1, color});
}

void appendUiText(
    novacore::render::RenderFrameInfo& frame,
    float x,
    float y,
    float scale,
    UiColor color,
    std::string text) {
    frame.debugTexts.push_back(novacore::render::DebugText{x, y, scale, color, std::move(text)});
}

} // namespace nemisis::ui
