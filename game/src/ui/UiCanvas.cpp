#include "nemisis/ui/UiCanvas.hpp"

#include <algorithm>
#include <string_view>
#include <utility>

namespace nemisis::ui {

namespace {

[[nodiscard]] float clampedProgress(float value) {
    return std::clamp(value, 0.0F, 1.0F);
}

[[nodiscard]] UiColor faded(UiColor color, float alphaScale) {
    color[3] = std::clamp(color[3] * alphaScale, 0.0F, 1.0F);
    return color;
}

[[nodiscard]] UiRect inset(UiRect rect, float amount) {
    return UiRect{
        rect.x + amount,
        rect.y + amount,
        std::max(0.0F, rect.width - (amount * 2.0F)),
        std::max(0.0F, rect.height - (amount * 2.0F)),
    };
}

[[nodiscard]] float textWidth(std::string_view text, float scale) {
    float longest = 0.0F;
    float current = 0.0F;
    for (const char c : text) {
        if (c == '\n') {
            longest = std::max(longest, current);
            current = 0.0F;
            continue;
        }
        current += std::max(1.0F, scale) * 6.0F;
    }
    return std::max(longest, current);
}

[[nodiscard]] float textLineCount(std::string_view text) {
    float lines = 1.0F;
    for (const char c : text) {
        if (c == '\n') {
            lines += 1.0F;
        }
    }
    return lines;
}

void appendRectPrimitive(novacore::render::RenderFrameInfo& frame, UiRect rect, UiColor color) {
    if (rect.width <= 0.0F || rect.height <= 0.0F || color[3] <= 0.0F) {
        return;
    }

    frame.debugRects.push_back(novacore::render::DebugRect{
        rect.x,
        rect.y,
        rect.width,
        rect.height,
        color,
    });
}

void appendRoundedRectPrimitive(novacore::render::RenderFrameInfo& frame, UiRect rect, float radius, UiColor color) {
    const float maxRadius = std::min(rect.width, rect.height) * 0.5F;
    radius = std::clamp(radius, 0.0F, maxRadius);
    if (radius <= 0.5F) {
        appendRectPrimitive(frame, rect, color);
        return;
    }

    appendRectPrimitive(
        frame,
        UiRect{rect.x + radius, rect.y, std::max(0.0F, rect.width - (radius * 2.0F)), rect.height},
        color);
    appendRectPrimitive(
        frame,
        UiRect{rect.x, rect.y + radius, radius, std::max(0.0F, rect.height - (radius * 2.0F))},
        color);
    appendRectPrimitive(
        frame,
        UiRect{rect.x + rect.width - radius, rect.y + radius, radius, std::max(0.0F, rect.height - (radius * 2.0F))},
        color);
}

void appendCommandToFrame(const UiCommand& command, novacore::render::RenderFrameInfo& frame) {
    switch (command.kind) {
    case UiCommandKind::Rect:
        appendUiRect(frame, command.rect, command.color);
        break;
    case UiCommandKind::RoundedRect:
        appendRoundedRectPrimitive(frame, command.rect, command.radius, command.color);
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
    case UiCommandKind::Image:
        appendUiRect(frame, command.rect, command.secondaryColor);
        appendUiRect(
            frame,
            UiRect{
                command.rect.x + 2.0F,
                command.rect.y + 2.0F,
                std::max(0.0F, command.rect.width - 4.0F),
                std::max(0.0F, command.rect.height - 4.0F),
            },
            command.color);
        appendUiText(
            frame,
            command.rect.x + 8.0F,
            command.rect.y + std::max(18.0F, command.rect.height - 18.0F),
            1.0F,
            palette::TextPrimary,
            command.assetId);
        break;
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

void UiCanvas::image(UiRect rect, std::string assetId, UiColor tint) {
    UiCommand command{};
    command.kind = UiCommandKind::Image;
    command.rect = rect;
    command.assetId = std::move(assetId);
    command.color = tint;
    command.secondaryColor = palette::Blueprint;
    commands_.push_back(std::move(command));
}

void UiCanvas::shadowedText(
    float x,
    float y,
    float scale,
    UiColor color,
    std::string textValue,
    UiColor shadow,
    float offset) {
    const float safeOffset = std::max(0.0F, offset);
    text(x + safeOffset, y + safeOffset, scale, shadow, textValue);
    text(x, y, scale, color, std::move(textValue));
}

void UiCanvas::outlinedText(float x, float y, float scale, UiColor color, std::string textValue, UiColor outline) {
    const float offset = std::max(1.0F, scale * 0.45F);
    text(x - offset, y, scale, outline, textValue);
    text(x + offset, y, scale, outline, textValue);
    text(x, y - offset, scale, outline, textValue);
    text(x, y + offset, scale, outline, textValue);
    text(x, y, scale, color, std::move(textValue));
}

void UiCanvas::panel(UiRect rect, UiPanelStyle style) {
    const float radius = std::max(0.0F, style.radius);
    roundedRect(rect, radius, style.fill);
    if (style.showBorder && style.borderWidth > 0.0F) {
        const float width = std::max(1.0F, style.borderWidth);
        const auto topLeft = inset(rect, width * 0.5F);
        line(topLeft.x + radius, topLeft.y, topLeft.x + topLeft.width - radius, topLeft.y, style.border);
        line(topLeft.x + radius, topLeft.y + topLeft.height, topLeft.x + topLeft.width - radius, topLeft.y + topLeft.height, style.border);
        line(topLeft.x, topLeft.y + radius, topLeft.x, topLeft.y + topLeft.height - radius, style.border);
        line(topLeft.x + topLeft.width, topLeft.y + radius, topLeft.x + topLeft.width, topLeft.y + topLeft.height - radius, style.border);
    }
    if (style.showAccent) {
        roundedRect(
            UiRect{rect.x, rect.y + style.radius, 4.0F, std::max(0.0F, rect.height - (style.radius * 2.0F))},
            2.0F,
            style.accent);
    }
}

void UiCanvas::button(UiRect rect, std::string label, std::string value, bool selected, UiButtonStyle style) {
    UiPanelStyle panelStyle{};
    panelStyle.fill = selected ? style.selectedFill : style.fill;
    panelStyle.border = selected ? style.accent : style.border;
    panelStyle.accent = style.accent;
    panelStyle.radius = style.radius;
    panelStyle.borderWidth = selected ? 2.0F : 1.0F;
    panelStyle.showAccent = selected;
    panel(rect, panelStyle);

    const float labelScale = fitTextScale(label, std::max(80.0F, rect.width * 0.52F), 2.0F, 1.0F);
    shadowedText(
        rect.x + 18.0F,
        rect.y + std::max(8.0F, (rect.height - (labelScale * 7.0F)) * 0.5F),
        labelScale,
        selected ? style.text : style.mutedText,
        std::move(label),
        {0.0F, 0.0F, 0.0F, selected ? 0.78F : 0.55F},
        selected ? 2.0F : 1.0F);

    if (!value.empty()) {
        const float valueScale = fitTextScale(value, std::max(72.0F, rect.width * 0.36F), 2.0F, 0.85F);
        const auto metrics = measureText(value, valueScale);
        shadowedText(
            rect.x + rect.width - metrics.width - 18.0F,
            rect.y + std::max(8.0F, (rect.height - metrics.height) * 0.5F),
            valueScale,
            style.text,
            std::move(value),
            {0.0F, 0.0F, 0.0F, 0.65F},
            1.0F);
    }
}

void UiCanvas::pill(UiRect rect, std::string label, UiColor fill, UiColor textColor) {
    UiPanelStyle style{};
    style.fill = fill;
    style.border = faded(textColor, 0.35F);
    style.radius = rect.height * 0.5F;
    style.borderWidth = 1.0F;
    panel(rect, style);
    const float scale = fitTextScale(label, rect.width - 18.0F, 1.0F, 0.65F);
    const auto metrics = measureText(label, scale);
    text(
        rect.x + ((rect.width - metrics.width) * 0.5F),
        rect.y + ((rect.height - metrics.height) * 0.5F),
        scale,
        textColor,
        std::move(label));
}

void UiCanvas::divider(float x0, float y0, float x1, float y1, UiColor color) {
    line(x0, y0, x1, y1, color);
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

UiTextMetrics UiCanvas::measureText(std::string_view textValue, float scale) {
    const float safeScale = std::max(0.1F, scale);
    return UiTextMetrics{
        textWidth(textValue, safeScale),
        textLineCount(textValue) * safeScale * 7.0F,
        safeScale * 8.0F,
    };
}

float UiCanvas::fitTextScale(std::string_view textValue, float maxWidth, float preferredScale, float minScale) {
    const float safePreferred = std::max(0.1F, preferredScale);
    const float width = measureText(textValue, safePreferred).width;
    if (width <= std::max(1.0F, maxWidth)) {
        return safePreferred;
    }
    const float scaled = safePreferred * (std::max(1.0F, maxWidth) / std::max(1.0F, width));
    return std::clamp(scaled, std::max(0.1F, minScale), safePreferred);
}

void appendUiRect(novacore::render::RenderFrameInfo& frame, UiRect rect, UiColor color) {
    appendRectPrimitive(frame, rect, color);
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
