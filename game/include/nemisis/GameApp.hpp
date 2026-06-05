#pragma once

#include "nemisis/dev/DevSandbox.hpp"
#include "nemisis/movement/MovementConfig.hpp"
#include "nemisis/movement/MovementSystem.hpp"
#include "nemisis/player/PlayerCommandQueue.hpp"
#include "nemisis/weapons/WeaponSystem.hpp"

#include "novacore/core/Application.hpp"
#include "novacore/core/ConfigRegistry.hpp"
#include "novacore/ecs/World.hpp"
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
    [[nodiscard]] bool isHeadless() const;

private:
    void applyConfig(std::string_view name);
    void applyLoadedConfigs();
    void ensureActiveWeapon(weapons::WeaponRuntimeState& weaponState, std::string_view requestedWeaponId);
    void ensureLocalPlayer();

    novacore::platform::InputSystem input_;
    novacore::platform::InputActionMap actions_;
    novacore::core::ConfigRegistry configRegistry_;
    novacore::platform::Window window_;
    novacore::render::Renderer renderer_;
    novacore::ecs::World world_;
    dev::DevSandbox devSandbox_;
    movement::MovementSystem movement_;
    player::PlayerCommandQueue localCommandQueue_;
    novacore::ecs::EntityId localPlayerEntity_;
    weapons::WeaponSystem weapons_;
};

} // namespace nemisis::game
