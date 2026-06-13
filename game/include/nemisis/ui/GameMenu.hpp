#pragma once

#include "nemisis/assets/DevAssetBindings.hpp"
#include "nemisis/dev/DevRangeRenderScene.hpp"
#include "nemisis/dev/DevSandbox.hpp"
#include "nemisis/dev/GreyboxWorld.hpp"
#include "nemisis/player/PlayerProfile.hpp"
#include "nemisis/settings/GameSettings.hpp"
#include "nemisis/weapons/WeaponAttachments.hpp"

#include "novacore/platform/InputAction.hpp"
#include "novacore/render/Renderer.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace nemisis::ui {

enum class GameScreen {
    MainMenu,
    Loading,
    DevRange,
    TeamDeathmatch,
    Control,
};

enum class MenuTab {
    Play,
    Gamemodes,
    Loadout,
    Character,
    Settings,
    Account
};

enum class DebugPage {
    Gameplay,
    Network,
    Assets
};

struct MenuPointerState final {
    float x = 0.0F;
    float y = 0.0F;
    bool available = false;
    bool primaryDown = false;
    bool primaryPressed = false;
    bool primaryReleased = false;
};

class GameMenu final {
public:
    void update(const novacore::platform::InputActionMap& actions);
    void update(
        const novacore::platform::InputActionMap& actions,
        settings::GameSettings& settings,
        weapons::WeaponLoadout& loadout,
        const weapons::AttachmentRegistry& attachments);
    void update(
        const novacore::platform::InputActionMap& actions,
        settings::GameSettings& settings,
        weapons::WeaponLoadout& loadout,
        const weapons::AttachmentRegistry& attachments,
        MenuPointerState pointer);
    void updateFrame(double deltaSeconds);
    void showMainMenu(MenuTab tab = MenuTab::Play);
    void showLoadingScreen(GameScreen target, float progress = 0.0F);
    void showDevRange();
    void showTeamDeathmatch();
    void showControl();
    void appendRenderCommands(
        novacore::render::RenderFrameInfo& frame,
        const nemisis::dev::DevSandboxSample& sample,
        const nemisis::dev::GreyboxWorld& greyboxWorld,
        std::string_view rendererBackend,
        std::string_view vulkanSummary,
        std::size_t queuedAssets,
        const nemisis::assets::DevAssetBindingSummary& assetSummary,
        const novacore::render::MeshResourceStats& meshStats,
        const novacore::render::RenderBackendFrameStats& backendFrameStats,
        const nemisis::dev::DevRangeRenderSceneStats& sceneStats,
        const settings::GameSettings& settings,
        const weapons::WeaponLoadout& loadout,
        const weapons::AttachmentRegistry& attachments,
        const weapons::AttachmentBuildSummary& attachmentSummary,
        const player::AccountStats& accountStats) const;

    [[nodiscard]] GameScreen screen() const;
    [[nodiscard]] MenuTab tab() const;
    [[nodiscard]] bool gameplayActive() const;
    [[nodiscard]] bool debugOverlayEnabled() const;
    [[nodiscard]] DebugPage debugPage() const;
    [[nodiscard]] float loadingProgress() const;
    [[nodiscard]] std::size_t selectedIndex() const;
    [[nodiscard]] weapons::AttachmentSlot selectedAttachmentSlot() const;
    [[nodiscard]] std::string title() const;
    [[nodiscard]] std::string_view screenName() const;
    [[nodiscard]] std::string_view tabName() const;
    [[nodiscard]] std::string_view debugPageName() const;
    [[nodiscard]] std::array<float, 4> clearColor() const;

private:
    void activateSelection(
        settings::GameSettings& settings,
        weapons::WeaponLoadout& loadout,
        const weapons::AttachmentRegistry& attachments);
    void setScreen(GameScreen screen);
    void startLoading(GameScreen target);
    void setTab(MenuTab tab);
    void adjustSelection(int delta);
    void adjustCurrentValue(
        int direction,
        settings::GameSettings& settings,
        weapons::WeaponLoadout& loadout,
        const weapons::AttachmentRegistry& attachments);
    void updatePointer(
        MenuPointerState pointer,
        settings::GameSettings& settings,
        weapons::WeaponLoadout& loadout,
        const weapons::AttachmentRegistry& attachments);
    [[nodiscard]] std::size_t itemCountForTab() const;
    void cycleDebugPage();

    GameScreen screen_ = GameScreen::MainMenu;
    GameScreen loadingTarget_ = GameScreen::DevRange;
    MenuTab tab_ = MenuTab::Play;
    DebugPage debugPage_ = DebugPage::Gameplay;
    std::size_t selectedIndex_ = 0;
    float loadingProgress_ = 0.0F;
    bool debugOverlayEnabled_ = true;
};

} // namespace nemisis::ui
