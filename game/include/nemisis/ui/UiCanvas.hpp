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
    Crosshair,
    Image
};

enum class UiTextAlign {
    Left,
    Center,
    Right,
};

struct UiTextMetrics final {
    float width = 0.0F;
    float height = 0.0F;
    float lineHeight = 0.0F;
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
    std::string assetId;
    std::string text;
};

struct UiFrameDesc final {
    float width = 1280.0F;
    float height = 720.0F;
    float dpiScale = 1.0F;
};

struct UiPanelStyle final {
    UiColor fill{0.020F, 0.032F, 0.038F, 0.94F};
    UiColor border{0.10F, 0.18F, 0.20F, 0.65F};
    UiColor accent{0.05F, 0.82F, 0.95F, 1.0F};
    float radius = 8.0F;
    float borderWidth = 1.0F;
    bool showBorder = true;
    bool showAccent = false;
};

struct UiButtonStyle final {
    UiColor fill{0.035F, 0.050F, 0.057F, 0.88F};
    UiColor selectedFill{0.0F, 0.35F, 0.48F, 0.95F};
    UiColor border{0.14F, 0.27F, 0.30F, 0.78F};
    UiColor accent{0.05F, 0.82F, 0.95F, 1.0F};
    UiColor text{0.90F, 0.96F, 0.98F, 1.0F};
    UiColor mutedText{0.52F, 0.64F, 0.70F, 1.0F};
    float radius = 8.0F;
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
    void image(UiRect rect, std::string assetId, UiColor tint = {1.0F, 1.0F, 1.0F, 1.0F});
    void shadowedText(
        float x,
        float y,
        float scale,
        UiColor color,
        std::string text,
        UiColor shadow = {0.0F, 0.0F, 0.0F, 0.72F},
        float offset = 2.0F);
    void outlinedText(float x, float y, float scale, UiColor color, std::string text, UiColor outline = {0.0F, 0.0F, 0.0F, 0.80F});
    void panel(UiRect rect, UiPanelStyle style);
    void button(UiRect rect, std::string label, std::string value, bool selected, UiButtonStyle style = {});
    void pill(UiRect rect, std::string label, UiColor fill, UiColor textColor);
    void divider(float x0, float y0, float x1, float y1, UiColor color = {0.12F, 0.20F, 0.22F, 0.85F});

    void appendToRenderFrame(novacore::render::RenderFrameInfo& frame) const;

    [[nodiscard]] const std::vector<UiCommand>& commands() const;
    [[nodiscard]] std::size_t commandCount() const;
    [[nodiscard]] UiFrameDesc frameDesc() const;
    [[nodiscard]] static UiTextMetrics measureText(std::string_view text, float scale);
    [[nodiscard]] static float fitTextScale(std::string_view text, float maxWidth, float preferredScale, float minScale = 0.7F);

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
inline constexpr UiColor Blueprint{0.075F, 0.15F, 0.19F, 0.92F};
inline constexpr UiColor Success{0.28F, 0.88F, 0.55F, 1.0F};

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
