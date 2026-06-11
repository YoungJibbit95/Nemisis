#pragma once

#include "novacore/render/Renderer.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace nemisis::ui {

using UiColor = std::array<float, 4>;

struct UiRect final {
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;
};

enum class UiCommandKind {
    Rect,
    RoundedRect,
    Line,
    Text,
    ProgressBar,
    Crosshair
};

struct UiCommand final {
    UiCommandKind kind = UiCommandKind::Rect;
    UiRect rect{};
    float x0 = 0.0F;
    float y0 = 0.0F;
    float x1 = 0.0F;
    float y1 = 0.0F;
    float radius = 0.0F;
    float scale = 1.0F;
    float value = 0.0F;
    UiColor color{1.0F, 1.0F, 1.0F, 1.0F};
    UiColor secondaryColor{0.0F, 0.0F, 0.0F, 1.0F};
    std::string text;
};

struct UiFrameDesc final {
    float width = 1280.0F;
    float height = 720.0F;
    float dpiScale = 1.0F;
};

class UiCanvas final {
public:
    void beginFrame(UiFrameDesc desc = {});
    void clear();

    void rect(UiRect rect, UiColor color);
    void roundedRect(UiRect rect, float radius, UiColor color);
    void line(float x0, float y0, float x1, float y1, UiColor color);
    void text(float x, float y, float scale, UiColor color, std::string text);
    void progressBar(UiRect rect, float value, UiColor background, UiColor foreground);
    void crosshair(float centerX, float centerY, float gap, float length, UiColor color);

    void appendToRenderFrame(novacore::render::RenderFrameInfo& frame) const;

    [[nodiscard]] const std::vector<UiCommand>& commands() const;
    [[nodiscard]] std::size_t commandCount() const;
    [[nodiscard]] UiFrameDesc frameDesc() const;

private:
    UiFrameDesc frame_{};
    std::vector<UiCommand> commands_;
};

namespace palette {

inline constexpr UiColor Panel{0.020F, 0.032F, 0.038F, 0.94F};
inline constexpr UiColor PanelRaised{0.040F, 0.058F, 0.066F, 0.96F};
inline constexpr UiColor Accent{0.05F, 0.82F, 0.95F, 1.0F};
inline constexpr UiColor AccentSoft{0.0F, 0.35F, 0.48F, 0.95F};
inline constexpr UiColor TextPrimary{0.90F, 0.96F, 0.98F, 1.0F};
inline constexpr UiColor TextSecondary{0.52F, 0.64F, 0.70F, 1.0F};
inline constexpr UiColor Warning{0.95F, 0.68F, 0.22F, 1.0F};
inline constexpr UiColor Danger{0.85F, 0.14F, 0.10F, 1.0F};
inline constexpr UiColor TransparentBlack{0.0F, 0.0F, 0.0F, 0.05F};

} // namespace palette

void appendUiRect(novacore::render::RenderFrameInfo& frame, UiRect rect, UiColor color);
void appendUiLine(novacore::render::RenderFrameInfo& frame, float x0, float y0, float x1, float y1, UiColor color);
void appendUiText(
    novacore::render::RenderFrameInfo& frame,
    float x,
    float y,
    float scale,
    UiColor color,
    std::string text);

} // namespace nemisis::ui
