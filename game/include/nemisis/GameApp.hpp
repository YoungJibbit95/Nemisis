#pragma once

#include "nemisis/assets/DevAssetBindings.hpp"
#include "nemisis/assets/GameAssetCatalog.hpp"
#include "nemisis/dev/DebugTarget.hpp"
#include "nemisis/dev/DevRangeRenderScene.hpp"
#include "nemisis/dev/DevSandbox.hpp"
#include "nemisis/dev/GreyboxWorld.hpp"
#include "nemisis/movement/MovementConfig.hpp"
#include "nemisis/movement/MovementSystem.hpp"
#include "nemisis/net/LoopbackCommandBridge.hpp"
#include "nemisis/player/PlayerCommandQueue.hpp"
#include "nemisis/render/RenderTuning.hpp"
#include "nemisis/ui/GameMenu.hpp"
#include "nemisis/weapons/WeaponSystem.hpp"

#include "novacore/core/Application.hpp"
#include "novacore/core/ConfigRegistry.hpp"
#include "novacore/ecs/World.hpp"
#include "novacore/assets/AssetStreamer.hpp"
#include "novacore/platform/InputAction.hpp"
#include "novacore/platform/Input.hpp"
#include "novacore/platform/Window.hpp"
#include "novacore/render/Renderer.hpp"

#include <array>
#include <string>
#include <string_view>
#include <unordered_map>

namespace nemisis::game {

struct GameAppOptions final {
    bool preferVulkanRenderer = true;
    bool requireVulkanRenderer = true;
    bool autoEnterDevRange = true;
    bool lockDevRange = true;
};

class GameApp final : public novacore::core::IApplicationDelegate {
public:
    explicit GameApp(GameAppOptions options = {});

    void onStartup() override;
    void onShutdown() override;
    void onFixedTick(const novacore::core::FrameContext& context) override;
    void onFrame(const novacore::core::FrameContext& context) override;
    [[nodiscard]] bool shouldQuit() const override;
    [[nodiscard]] bool isHeadless() const override;

private:
    void loadAssetCatalog();
    void applyConfig(std::string_view name);
    void applyLoadedConfigs();
    void ensureActiveWeapon(weapons::WeaponRuntimeState& weaponState, std::string_view requestedWeaponId);
    void ensureLocalPlayer();
    void syncRelativeMouseMode();
    void registerDevMeshResources();
    void releaseDevMeshResources();
    void appendA0MeshWireframePreview(novacore::render::RenderFrameInfo& frame) const;
    [[nodiscard]] dev::DevRangePlayerRenderState currentPlayerRenderState() const;

    GameAppOptions options_;
    novacore::platform::InputSystem input_;
    novacore::platform::InputActionMap actions_;
    novacore::core::ConfigRegistry configRegistry_;
    novacore::platform::Window window_;
    novacore::render::Renderer renderer_;
    novacore::ecs::World world_;
    nemisis::assets::GameAssetCatalog assetCatalog_;
    novacore::assets::AssetStreamer assetStreamer_;
    nemisis::assets::DevAssetBindings devAssetBindings_;
    nemisis::assets::DevAssetBindingSummary devAssetSummary_;
    std::unordered_map<std::string, novacore::render::MeshResourceHandle> devMeshResources_;
    novacore::ecs::EntityId cameraEntity_;
    dev::DebugTargetState debugTarget_;
    dev::DevSandbox devSandbox_;
    dev::GreyboxWorld greyboxWorld_ = dev::createDevRangeGreyboxWorld();
    dev::DevRangeRenderSceneBuilder devRangeRenderer_;
    dev::DevRangeRenderSceneStats latestDevRangeRenderStats_;
    ui::GameMenu menu_;
    movement::MovementSystem movement_;
    render::DevRenderTuning renderTuning_;
    net::LoopbackCommandBridge loopbackBridge_;
    player::PlayerCommandQueue localCommandQueue_;
    novacore::ecs::EntityId localPlayerEntity_;
    weapons::WeaponSystem weapons_;
    float debugTargetRespawnSeconds_ = 0.0F;
    bool relativeMouseDesired_ = false;
};

} // namespace nemisis::game
