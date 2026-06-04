#include "nemisis/GameApp.hpp"

#include "nemisis/input/InputBindings.hpp"
#include "nemisis/input/InputCommandBuilder.hpp"
#include "nemisis/weapons/WeaponSimulation.hpp"

#include "novacore/core/Log.hpp"

#include <algorithm>
#include <array>
#include <string_view>

namespace nemisis::game {

namespace {

constexpr std::string_view kDefaultWeaponId = "ar_01";

} // namespace

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
    ensureActiveWeapon();

    novacore::core::logInfo("game", "Nemisis sandbox camera entity created");
    novacore::core::logInfo("game", "Prototype weapon registry initialized");
    novacore::core::logInfo("game", "Default input action map and config registry initialized");
}

void GameApp::onShutdown() {
    renderer_.shutdown();
    window_.shutdown();
}

void GameApp::onFixedTick(const novacore::core::FrameContext& context) {
    const auto command = input::buildPlayerInputCommand(actions_, context.tickIndex);
    localMovementState_ = movement_.simulate(localMovementState_, command, static_cast<float>(context.fixedDeltaSeconds));

    ensureActiveWeapon();
    if (const auto* weapon = weapons_.findWeapon(activeWeapon_.weaponId); weapon != nullptr) {
        weapons::FireRequest fireRequest{};
        fireRequest.triggerHeld = command.fireHeld;
        fireRequest.reloadPressed = command.reloadPressed;
        fireRequest.fixedDeltaSeconds = static_cast<float>(context.fixedDeltaSeconds);
        (void)weapons::simulateWeaponTick(*weapon, activeWeapon_, fireRequest);
    }
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
        ensureActiveWeapon();
    }
}

void GameApp::applyLoadedConfigs() {
    applyConfig("movement");
    applyConfig("weapons");
}

void GameApp::ensureActiveWeapon() {
    const std::string_view requestedWeaponId =
        activeWeapon_.weaponId.empty() ? kDefaultWeaponId : std::string_view(activeWeapon_.weaponId);

    const auto* weapon = weapons_.findWeapon(requestedWeaponId);
    if (weapon == nullptr) {
        weapon = weapons_.findWeapon(kDefaultWeaponId);
    }
    if (weapon == nullptr) {
        return;
    }

    if (activeWeapon_.weaponId != weapon->id) {
        activeWeapon_ = weapons::WeaponRuntimeState{};
        activeWeapon_.weaponId = weapon->id;
        activeWeapon_.ammoInMagazine = weapon->magazineSize;
        return;
    }

    activeWeapon_.ammoInMagazine = std::min(activeWeapon_.ammoInMagazine, weapon->magazineSize);
    activeWeapon_.reloadTimeRemaining = std::min(activeWeapon_.reloadTimeRemaining, weapon->reloadTimeSeconds);
}

} // namespace nemisis::game
