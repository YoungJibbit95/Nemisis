#include "nemisis/GameApp.hpp"

#include "novacore/core/Log.hpp"

#include <array>

namespace nemisis::game {

void GameApp::onStartup() {
    actions_ = input::createDefaultActionMap();
    configTracker_.track("configs/input/default_input.json");
    configTracker_.track("configs/movement/player_movement.json");
    configTracker_.track("configs/weapons/core_trio.json");
    configTracker_.track("configs/game_modes/tdm_control.json");

    novacore::platform::WindowDesc windowDesc{};
    windowDesc.title = "Nemisis - M1 Thin Spine";
    windowDesc.width = 1280;
    windowDesc.height = 720;
    windowDesc.preferVulkan = true;

    window_.create(windowDesc);

    novacore::render::RendererCreateInfo rendererInfo{};
    renderer_.create(window_, rendererInfo);

    const auto camera = world_.createEntity();
    world_.addComponent(camera, novacore::ecs::NameComponent{"main_camera"});
    world_.addComponent(camera, novacore::ecs::TransformComponent{});
    world_.addComponent(camera, novacore::ecs::CameraComponent{});

    weapons_.registerPrototypeLoadout();

    novacore::core::logInfo("game", "Nemisis sandbox camera entity created");
    novacore::core::logInfo("game", "Prototype weapon registry initialized");
    novacore::core::logInfo("game", "Default input action map and config tracking initialized");
}

void GameApp::onShutdown() {
    renderer_.shutdown();
    window_.shutdown();
}

void GameApp::onFixedTick(const novacore::core::FrameContext& context) {
    nemisis::player::PlayerInputCommand command{};
    command.tick = context.tickIndex;

    localMovementState_ = movement_.simulate(localMovementState_, command, static_cast<float>(context.fixedDeltaSeconds));
}

void GameApp::onFrame(const novacore::core::FrameContext& context) {
    (void)context;
    window_.pollEvents(input_);

    for (const auto& change : configTracker_.pollChanges()) {
        if (change.contentMayHaveChanged) {
            novacore::core::logInfo("game", "Detected config file change");
        }
    }

    novacore::render::RenderFrameInfo frameInfo{};
    frameInfo.clearColor = std::array<float, 4>{0.025F, 0.035F, 0.055F, 1.0F};
    renderer_.beginFrame(frameInfo);
    renderer_.endFrame();
}

bool GameApp::shouldQuit() const {
    return window_.shouldClose();
}

bool GameApp::isHeadless() const {
    return window_.isHeadless();
}

} // namespace nemisis::game
