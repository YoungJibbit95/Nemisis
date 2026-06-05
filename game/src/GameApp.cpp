#include "nemisis/GameApp.hpp"

#include "nemisis/input/InputBindings.hpp"
#include "nemisis/input/InputCommandBuilder.hpp"
#include "nemisis/player/PlayerComponents.hpp"
#include "nemisis/player/PlayerSpawn.hpp"
#include "nemisis/weapons/WeaponSimulation.hpp"

#include "novacore/core/Log.hpp"
#include "novacore/ecs/Components.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
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
    ensureLocalPlayer();

    novacore::core::logInfo("game", "Nemisis sandbox camera entity created");
    novacore::core::logInfo("game", "Prototype weapon registry initialized");
    novacore::core::logInfo("game", "Default input action map and config registry initialized");
}

void GameApp::onShutdown() {
    renderer_.shutdown();
    window_.shutdown();
}

void GameApp::onFixedTick(const novacore::core::FrameContext& context) {
    ensureLocalPlayer();

    const auto command = input::buildPlayerInputCommand(actions_, context.tickIndex);
    (void)localCommandQueue_.push(command);

    auto* movementState = world_.getComponent<movement::PlayerMovementState>(localPlayerEntity_);
    if (movementState != nullptr) {
        *movementState = movement_.simulate(*movementState, command, static_cast<float>(context.fixedDeltaSeconds));
        if (auto* transform = world_.getComponent<novacore::ecs::TransformComponent>(localPlayerEntity_);
            transform != nullptr) {
            transform->position = movementState->position;
        }
    }

    auto* weaponState = world_.getComponent<weapons::WeaponRuntimeState>(localPlayerEntity_);
    const auto* loadout = world_.getComponent<player::PlayerLoadoutComponent>(localPlayerEntity_);
    if (weaponState != nullptr) {
        ensureActiveWeapon(
            *weaponState,
            loadout != nullptr ? std::string_view(loadout->activeWeaponId) : kDefaultWeaponId);
    }

    if (weaponState != nullptr) {
        const auto* weapon = weapons_.findWeapon(weaponState->weaponId);
        if (weapon != nullptr) {
            weapons::FireRequest fireRequest{};
            fireRequest.triggerHeld = command.fireHeld;
            fireRequest.reloadPressed = command.reloadPressed;
            fireRequest.fixedDeltaSeconds = static_cast<float>(context.fixedDeltaSeconds);
            (void)weapons::simulateWeaponTick(*weapon, *weaponState, fireRequest);
        }
    }

    if (auto* network = world_.getComponent<player::PlayerNetworkComponent>(localPlayerEntity_);
        network != nullptr) {
        network->lastProcessedCommandTick = command.tick;
        network->pendingCommandCount = static_cast<std::uint16_t>(
            std::min<std::size_t>(localCommandQueue_.size(), 65535));
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
        if (!localPlayerEntity_.isNull() && world_.isAlive(localPlayerEntity_)) {
            auto* weaponState = world_.getComponent<weapons::WeaponRuntimeState>(localPlayerEntity_);
            const auto* loadout = world_.getComponent<player::PlayerLoadoutComponent>(localPlayerEntity_);
            if (weaponState != nullptr) {
                ensureActiveWeapon(
                    *weaponState,
                    loadout != nullptr ? std::string_view(loadout->activeWeaponId) : kDefaultWeaponId);
            }
        }
    }
}

void GameApp::applyLoadedConfigs() {
    applyConfig("movement");
    applyConfig("weapons");
}

void GameApp::ensureActiveWeapon(weapons::WeaponRuntimeState& weaponState, std::string_view fallbackWeaponId) {
    const std::string_view requestedWeaponId = fallbackWeaponId.empty() ? kDefaultWeaponId : fallbackWeaponId;
    const bool shouldUseRequestedWeapon =
        weaponState.weaponId.empty() || weaponState.weaponId != requestedWeaponId;
    const std::string_view desiredWeaponId =
        shouldUseRequestedWeapon ? requestedWeaponId : std::string_view(weaponState.weaponId);

    const auto* weapon = weapons_.findWeapon(desiredWeaponId);
    if (weapon == nullptr) {
        weapon = weapons_.findWeapon(kDefaultWeaponId);
    }
    if (weapon == nullptr) {
        return;
    }

    if (weaponState.weaponId != weapon->id) {
        weaponState = weapons::WeaponRuntimeState{};
        weaponState.weaponId = weapon->id;
        weaponState.ammoInMagazine = weapon->magazineSize;
        return;
    }

    weaponState.ammoInMagazine = std::min(weaponState.ammoInMagazine, weapon->magazineSize);
    weaponState.reloadTimeRemaining = std::min(weaponState.reloadTimeRemaining, weapon->reloadTimeSeconds);
}

void GameApp::ensureLocalPlayer() {
    if (!localPlayerEntity_.isNull() && world_.isAlive(localPlayerEntity_)) {
        return;
    }

    player::LocalPlayerSpawnDesc spawnDesc{};
    spawnDesc.activeWeaponId = std::string(kDefaultWeaponId);
    localPlayerEntity_ = player::spawnLocalPlayer(world_, spawnDesc, weapons_.findWeapon(spawnDesc.activeWeaponId));
    localCommandQueue_.clear();
    novacore::core::logInfo("game", "Local player entity spawned");
}

} // namespace nemisis::game
