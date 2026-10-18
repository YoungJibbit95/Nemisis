#pragma once
#include "novacore/platform/InputAction.hpp"
#include "novacore/render/UiDrawData.hpp"
#include <memory>
#include <string>

namespace nemisis::ui {
enum class ShowcaseAction { None, Menu, DevRange, Quit };
struct UiShowcaseStats {
    double updateMilliseconds = 0;
    double renderMilliseconds = 0;
    std::size_t drawCalls = 0;
    std::size_t vertices = 0;
    std::size_t textureBytes = 0;
    std::size_t warningCount = 0;
};
// Game-owned document/controller. All layout, focus, controls and transitions use
// RmlUi; the bridge only translates its geometry into NovaCore's public API.
class UiShowcase final {
public:
    UiShowcase();
    ~UiShowcase();
    void initialize(const std::string& assetRoot = "assets/ui");
    ShowcaseAction update(double delta, const novacore::platform::InputSnapshot& input,
        const novacore::platform::InputActionMap& actions, int width, int height);
    void render(novacore::render::UiDrawData& output);
    [[nodiscard]] UiShowcaseStats stats() const;
    // Same path as live input; used by integration tests without a window.
    void key(int key, bool down, int modifiers = 0);
    void pointer(int x, int y, bool down);
    [[nodiscard]] std::string value(const std::string& id) const;
    [[nodiscard]] std::string focusId() const;
    [[nodiscard]] std::array<float,4> bounds(const std::string& id) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
