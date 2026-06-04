#pragma once

#include "novacore/core/Application.hpp"
#include "novacore/ecs/World.hpp"
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
    novacore::platform::Window window_;
    novacore::render::Renderer renderer_;
    novacore::ecs::World world_;
};

} // namespace nemisis::game

