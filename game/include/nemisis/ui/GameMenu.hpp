#pragma once

#include "nemisis/dev/DevSandbox.hpp"

#include "novacore/platform/InputAction.hpp"
#include "novacore/render/Renderer.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace nemisis::ui {

enum class GameScreen {
    MainMenu,
    DevRange,
    TeamDeathmatch,
    Control,
};

class GameMenu final {
public:
    void update(const novacore::platform::InputActionMap& actions);
    void appendRenderCommands(
        novacore::render::RenderFrameInfo& frame,
        const nemisis::dev::DevSandboxSample& sample,
        std::string_view rendererBackend,
        std::size_t queuedAssets) const;

    [[nodiscard]] GameScreen screen() const;
    [[nodiscard]] bool gameplayActive() const;
    [[nodiscard]] bool debugOverlayEnabled() const;
    [[nodiscard]] std::string title() const;
    [[nodiscard]] std::string_view screenName() const;
    [[nodiscard]] std::array<float, 4> clearColor() const;

private:
    void activateSelection();
    void setScreen(GameScreen screen);

    GameScreen screen_ = GameScreen::MainMenu;
    std::size_t selectedIndex_ = 0;
    bool debugOverlayEnabled_ = true;
};

} // namespace nemisis::ui
