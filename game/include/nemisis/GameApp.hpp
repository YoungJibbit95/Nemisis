#pragma once

#include "nemisis/assets/GameAssetCatalog.hpp"
#include "nemisis/dev/DebugTarget.hpp"
#include "nemisis/dev/DevSandbox.hpp"
#include "nemisis/dev/GreyboxWorld.hpp"
#include "nemisis/movement/MovementConfig.hpp"
#include "nemisis/movement/MovementSystem.hpp"
#include "nemisis/net/LoopbackCommandBridge.hpp"
#include "nemisis/player/PlayerCommandQueue.hpp"
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

#include <string_view>

namespace nemisis::game {

class GameApp final : public novacore::core::IApplicationDelegate {
public:
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

    novacore::platform::InputSystem input_;
    novacore::platform::InputActionMap actions_;
    novacore::core::ConfigRegistry configRegistry_;
    novacore::platform::Window window_;
    novacore::render::Renderer renderer_;
    novacore::ecs::World world_;
    nemisis::assets::GameAssetCatalog assetCatalog_;
    novacore::assets::AssetStreamer assetStreamer_;
    novacore::ecs::EntityId cameraEntity_;
    dev::DebugTargetState debugTarget_;
    dev::DevSandbox devSandbox_;
    dev::GreyboxWorld greyboxWorld_ = dev::createDevRangeGreyboxWorld();
    ui::GameMenu menu_;
    movement::MovementSystem movement_;
    net::LoopbackCommandBridge loopbackBridge_;
    player::PlayerCommandQueue localCommandQueue_;
    novacore::ecs::EntityId localPlayerEntity_;
    weapons::WeaponSystem weapons_;
    float debugTargetRespawnSeconds_ = 0.0F;
    bool relativeMouseDesired_ = false;
};

} // namespace nemisis::game
