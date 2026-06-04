#include "nemisis/GameApp.hpp"

#include "novacore/core/Log.hpp"

#include <array>
#include <string_view>

namespace nemisis::game {

void GameApp::onStartup() {
    actions_ = input::createDefaultActionMap();
    configRegistry_.watchJson("input", "configs/input/default_input.json");
    configRegistry_.watchJson("movement", "configs/movement/player_movement.json");
    configRegistry_.watchJson("weapons", "configs/weapons/core_trio.json");
    configRegistry_.watchJson("modes", "configs/game_modes/tdm_control.json");
    applyLoadedConfigs();

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

    if (weapons_.weaponCount() == 0) {
        weapons_.registerPrototypeLoadout();
    }

    novacore::core::logInfo("game", "Nemisis sandbox camera entity created");
    novacore::core::logInfo("game", "Prototype weapon registry initialized");
    novacore::core::logInfo("game", "Default input action map and config registry initialized");
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

    for (const auto& event : configRegistry_.pollReloads()) {
        if (event.loaded) {
            applyConfig(event.name);
            novacore::core::logInfo("game", "Reloaded config: " + event.name);
        } else {
            novacore::core::logWarning("game", "Config reload failed: " + event.name);
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

void GameApp::applyConfig(std::string_view name) {
    if (name == "movement") {
        const auto* document = configRegistry_.find("movement");
        if (document != nullptr) {
            movement_.setTuning(movement::movementTuningFromConfig(*document, movement_.tuning()));
        }
        return;
    }

    if (name == "weapons") {
        const auto* document = configRegistry_.find("weapons");
        if (document != nullptr && !weapons_.loadFromConfig(*document)) {
            weapons_.registerPrototypeLoadout();
        }
    }
}

void GameApp::applyLoadedConfigs() {
    applyConfig("movement");
    applyConfig("weapons");
}

} // namespace nemisis::game
