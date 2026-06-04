#pragma once

#include "nemisis/input/InputBindings.hpp"
#include "nemisis/movement/MovementSystem.hpp"
#include "nemisis/weapons/WeaponSystem.hpp"

#include "novacore/core/Application.hpp"
#include "novacore/ecs/World.hpp"
#include "novacore/io/FileSystem.hpp"
#include "novacore/platform/Input.hpp"
#include "novacore/platform/Window.hpp"
#include "novacore/render/Renderer.hpp"

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
    novacore::platform::InputSystem input_;
    novacore::platform::InputActionMap actions_;
    novacore::io::FileChangeTracker configTracker_;
    novacore::platform::Window window_;
    novacore::render::Renderer renderer_;
    novacore::ecs::World world_;
    movement::MovementSystem movement_;
    movement::PlayerMovementState localMovementState_;
    weapons::WeaponSystem weapons_;
};

} // namespace nemisis::game
