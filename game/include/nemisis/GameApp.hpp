#pragma once

#include "nemisis/movement/MovementConfig.hpp"
#include "nemisis/movement/MovementSystem.hpp"
#include "nemisis/weapons/WeaponSystem.hpp"

#include "novacore/core/Application.hpp"
#include "novacore/core/ConfigRegistry.hpp"
#include "novacore/ecs/World.hpp"
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
    void ensureActiveWeapon();

    novacore::platform::InputSystem input_;
    novacore::platform::InputActionMap actions_;
    novacore::core::ConfigRegistry configRegistry_;
    novacore::platform::Window window_;
    novacore::render::Renderer renderer_;
    novacore::ecs::World world_;
    movement::MovementSystem movement_;
    movement::PlayerMovementState localMovementState_;
    weapons::WeaponSystem weapons_;
    weapons::WeaponRuntimeState activeWeapon_;
};

} // namespace nemisis::game
