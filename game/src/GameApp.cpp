#include "nemisis/GameApp.hpp"

#include "nemisis/dev/DebugTarget.hpp"
#include "nemisis/input/InputBindings.hpp"
#include "nemisis/input/InputCommandBuilder.hpp"
#include "nemisis/player/PlayerComponents.hpp"
#include "nemisis/player/PlayerSpawn.hpp"
#include "nemisis/player/PlayerView.hpp"
#include "nemisis/weapons/WeaponSimulation.hpp"
#include "nemisis/weapons/WeaponShot.hpp"

#include "novacore/core/Log.hpp"
#include "novacore/ecs/Components.hpp"
#include "novacore/math/Types.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

namespace nemisis::game {

namespace {

constexpr std::string_view kDefaultWeaponId = "ar_01";
constexpr float kDebugTargetRespawnDelaySeconds = 1.5F;

} // namespace

void GameApp::onStartup() {
    actions_ = input::createDefaultActionMap();
    configRegistry_.watchJson("input", "configs/input/default_input.json");
    configRegistry_.watchJson("movement", "configs/movement/player_movement.json");
    configRegistry_.watchJson("weapons", "configs/weapons/core_trio.json");
    configRegistry_.watchJson("modes", "configs/game_modes/tdm_control.json");
    applyLoadedConfigs();
    loadAssetCatalog();

    novacore::platform::WindowDesc windowDesc{};
    windowDesc.title = "Nemisis - M1 Thin Spine";
    windowDesc.width = 1280;
    windowDesc.height = 720;
    windowDesc.preferVulkan = true;

    window_.create(windowDesc);

    novacore::render::RendererCreateInfo rendererInfo{};
    renderer_.create(window_, rendererInfo);

    cameraEntity_ = world_.createEntity();
    world_.addComponent(cameraEntity_, novacore::ecs::NameComponent{"main_camera"});
    world_.addComponent(cameraEntity_, novacore::ecs::TransformComponent{});
    world_.addComponent(cameraEntity_, novacore::ecs::CameraComponent{});

    if (weapons_.weaponCount() == 0) {
        weapons_.registerPrototypeLoadout();
    }
    ensureLocalPlayer();

    novacore::core::logInfo("game", "Nemisis sandbox camera entity created");
    novacore::core::logInfo("game", "Prototype weapon registry initialized");
    novacore::core::logInfo("game", "Default input action map and config registry initialized");

    if (window_.isHeadless()) {
        novacore::core::logInfo("game", "Running in HEADLESS mode (no SDL3/Vulkan). Press Ctrl+C in your terminal/console to exit.");
    }
}

void GameApp::onShutdown() {
    renderer_.shutdown();
    window_.shutdown();
}

void GameApp::onFixedTick(const novacore::core::FrameContext& context) {
    ensureLocalPlayer();

    const auto command = input::buildPlayerInputCommand(actions_, context.tickIndex);
    (void)localCommandQueue_.push(command);

    player::PlayerId playerId = 1;
    if (const auto* identity = world_.getComponent<player::PlayerIdentityComponent>(localPlayerEntity_);
        identity != nullptr) {
        playerId = identity->playerId;
    }
    loopbackBridge_.sendPendingCommands(playerId, localCommandQueue_);
    loopbackBridge_.processServer(context.tickIndex);
    const auto acknowledgedCommands = loopbackBridge_.processClientAcks(localCommandQueue_);

    auto movementCommand = command;
    auto* view = world_.getComponent<player::PlayerViewComponent>(localPlayerEntity_);
    if (view != nullptr) {
        player::applyLook(*view, command.look);
        movementCommand = player::commandRelativeToView(command, *view);
    }

    auto* movementState = world_.getComponent<movement::PlayerMovementState>(localPlayerEntity_);
    if (movementState != nullptr) {
        *movementState = movement_.simulate(*movementState, movementCommand, static_cast<float>(context.fixedDeltaSeconds));
        if (auto* transform = world_.getComponent<novacore::ecs::TransformComponent>(localPlayerEntity_);
            transform != nullptr) {
            transform->position = movementState->position;
        }
        if (auto* cameraTransform = world_.getComponent<novacore::ecs::TransformComponent>(cameraEntity_);
            cameraTransform != nullptr) {
            cameraTransform->position = movementState->position + novacore::math::Vec3{0.0F, 1.65F, 0.0F};
        }
    }

    auto* weaponState = world_.getComponent<weapons::WeaponRuntimeState>(localPlayerEntity_);
    const auto* loadout = world_.getComponent<player::PlayerLoadoutComponent>(localPlayerEntity_);
    if (weaponState != nullptr) {
        ensureActiveWeapon(
            *weaponState,
            loadout != nullptr ? std::string_view(loadout->activeWeaponId) : kDefaultWeaponId);
    }

    weapons::FireResult fireResult{};
    weapons::ShotTraceResult shotTrace{};
    bool hasShotTrace = false;
    if (weaponState != nullptr) {
        const auto* weapon = weapons_.findWeapon(weaponState->weaponId);
        if (weapon != nullptr) {
            weapons::FireRequest fireRequest{};
            fireRequest.triggerHeld = command.fireHeld;
            fireRequest.reloadPressed = command.reloadPressed;
            fireRequest.fixedDeltaSeconds = static_cast<float>(context.fixedDeltaSeconds);
            fireResult = weapons::simulateWeaponTick(*weapon, *weaponState, fireRequest);
            if (fireResult.fired && movementState != nullptr) {
                weapons::ShotTraceRequest shotRequest{};
                shotRequest.origin = movementState->position + novacore::math::Vec3{0.0F, 1.65F, 0.0F};
                if (view != nullptr) {
                    shotRequest.forward = player::viewVectors(*view).forward;
                } else {
                    shotRequest.forward = novacore::math::Vec3{0.0F, 0.0F, 1.0F};
                }
                shotRequest.seed = static_cast<std::uint32_t>(command.tick);
                shotRequest.shotIndex = fireResult.shotIndex;
                shotRequest.movementSpeed =
                    std::sqrt((movementState->velocity.x * movementState->velocity.x) +
                              (movementState->velocity.z * movementState->velocity.z));
                shotRequest.ads = command.adsHeld;
                shotTrace = weapons::buildShotTrace(*weapon, shotRequest);
                hasShotTrace = true;
            }
        }
    }

    dev::DebugTargetHitResult targetHit{};
    if (hasShotTrace) {
        targetHit = dev::applyShotToDebugTarget(debugTarget_, shotTrace);
        if (targetHit.eliminated) {
            debugTargetRespawnSeconds_ = kDebugTargetRespawnDelaySeconds;
        }
    }

    if (debugTarget_.eliminated && debugTargetRespawnSeconds_ > 0.0F) {
        debugTargetRespawnSeconds_ = std::max(
            0.0F,
            debugTargetRespawnSeconds_ - static_cast<float>(context.fixedDeltaSeconds));
        if (debugTargetRespawnSeconds_ <= 0.0F) {
            dev::resetDebugTarget(debugTarget_);
        }
    }

    player::PlayerNetworkComponent networkSample{};
    if (auto* network = world_.getComponent<player::PlayerNetworkComponent>(localPlayerEntity_);
        network != nullptr) {
        network->lastProcessedCommandTick = command.tick;
        if (acknowledgedCommands > 0) {
            network->lastServerAcknowledgedTick = loopbackBridge_.stats().lastAcknowledgedTick;
        }
        network->pendingCommandCount = static_cast<std::uint16_t>(
            std::min<std::size_t>(localCommandQueue_.size(), 65535));
        networkSample = *network;
    }

    movement::PlayerMovementState movementSample{};
    if (movementState != nullptr) {
        movementSample = *movementState;
    }

    weapons::WeaponRuntimeState weaponSample{};
    if (weaponState != nullptr) {
        weaponSample = *weaponState;
    }

    devSandbox_.recordTick(dev::DevSandboxSample{
        command.tick,
        command,
        movementSample.position,
        movementSample.velocity,
        movementSample.mode,
        weaponSample,
        fireResult,
        shotTrace,
        hasShotTrace,
        debugTarget_,
        targetHit,
        networkSample,
        loopbackBridge_.stats(),
    });
}

void GameApp::onFrame(const novacore::core::FrameContext& context) {
    window_.pollEvents(input_);
    actions_.update(window_.inputSnapshot());

    for (const auto& event : configRegistry_.pollReloads()) {
        if (event.loaded) {
            applyConfig(event.name);
            novacore::core::logInfo("game", "Reloaded config: " + event.name);
        } else {
            novacore::core::logWarning("game", "Config reload failed: " + event.name);
        }
    }

    devSandbox_.onFrame(context.deltaSeconds);

    novacore::render::RenderFrameInfo frameInfo{};
    frameInfo.clearColor = devSandbox_.clearColor();
    renderer_.beginFrame(frameInfo);
    renderer_.endFrame();
}

bool GameApp::shouldQuit() const {
    return window_.shouldClose();
}

bool GameApp::isHeadless() const {
    return window_.isHeadless();
}

void GameApp::loadAssetCatalog() {
    const auto result = assetCatalog_.loadFromJson("configs/assets/nemisis_assets.json");
    if (!result.ok()) {
        for (const auto& error : result.errors) {
            novacore::core::logWarning("game", "Asset catalog load failed: " + error);
        }
        return;
    }

    const auto devZone = assetCatalog_.devSandboxStreamingZone();
    assetStreamer_.requestZone(devZone, 0);
    novacore::core::logInfo(
        "game",
        "Asset catalog loaded: " + std::to_string(assetCatalog_.assetCount()) + " assets");
    novacore::core::logInfo(
        "game",
        "Dev sandbox asset preload requests queued: " + std::to_string(assetStreamer_.pendingCount()));
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
