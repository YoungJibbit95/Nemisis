#pragma once

#include "nemisis/assets/DevAssetBindings.hpp"
#include "nemisis/dev/DevRangeRenderScene.hpp"
#include "nemisis/dev/DevSandbox.hpp"
#include "nemisis/dev/GreyboxWorld.hpp"

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

enum class DebugPage {
    Gameplay,
    Network,
    Assets
};

class GameMenu final {
public:
    void update(const novacore::platform::InputActionMap& actions);
    void showDevRange();
    void appendRenderCommands(
        novacore::render::RenderFrameInfo& frame,
        const nemisis::dev::DevSandboxSample& sample,
        const nemisis::dev::GreyboxWorld& greyboxWorld,
        std::string_view rendererBackend,
        std::string_view vulkanSummary,
        std::size_t queuedAssets,
        const nemisis::assets::DevAssetBindingSummary& assetSummary,
        const novacore::render::MeshResourceStats& meshStats,
        const nemisis::dev::DevRangeRenderSceneStats& sceneStats) const;

    [[nodiscard]] GameScreen screen() const;
    [[nodiscard]] bool gameplayActive() const;
    [[nodiscard]] bool debugOverlayEnabled() const;
    [[nodiscard]] DebugPage debugPage() const;
    [[nodiscard]] std::string title() const;
    [[nodiscard]] std::string_view screenName() const;
    [[nodiscard]] std::string_view debugPageName() const;
    [[nodiscard]] std::array<float, 4> clearColor() const;

private:
    void activateSelection();
    void setScreen(GameScreen screen);
    void cycleDebugPage();

    GameScreen screen_ = GameScreen::MainMenu;
    DebugPage debugPage_ = DebugPage::Gameplay;
    std::size_t selectedIndex_ = 0;
    bool debugOverlayEnabled_ = true;
};

} // namespace nemisis::ui
