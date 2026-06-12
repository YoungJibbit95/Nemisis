#include "nemisis/GameApp.hpp"

#include "nemisis/dev/DebugTarget.hpp"
#include "nemisis/dev/GreyboxCollision.hpp"
#include "nemisis/input/InputBindings.hpp"
#include "nemisis/input/InputCommandBuilder.hpp"
#include "nemisis/player/PlayerComponents.hpp"
#include "nemisis/player/PlayerHealth.hpp"
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
#include <limits>
#include <string>
#include <string_view>

namespace nemisis::game {

namespace {

constexpr std::string_view kDefaultWeaponId = "ar_01";

struct MeshPreviewBounds final {
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();

    void include(novacore::math::Vec3 value) {
        minX = std::min(minX, value.x);
        maxX = std::max(maxX, value.x);
        minZ = std::min(minZ, value.z);
        maxZ = std::max(maxZ, value.z);
    }

    [[nodiscard]] bool valid() const {
        return minX <= maxX && minZ <= maxZ;
    }
};

[[nodiscard]] std::array<float, 2> projectMeshPreview(
    const MeshPreviewBounds& bounds,
    novacore::math::Vec3 position) {
    constexpr float mapX = 58.0F;
    constexpr float mapY = 190.0F;
    constexpr float mapWidth = 520.0F;
    constexpr float mapHeight = 338.0F;

    const float spanX = std::max(0.001F, bounds.maxX - bounds.minX);
    const float spanZ = std::max(0.001F, bounds.maxZ - bounds.minZ);
    return {
        mapX + ((position.x - bounds.minX) / spanX) * mapWidth,
        mapY + mapHeight - (((position.z - bounds.minZ) / spanZ) * mapHeight),
    };
}

void addPreviewLine(
    novacore::render::RenderFrameInfo& frame,
    const MeshPreviewBounds& bounds,
    novacore::math::Vec3 a,
    novacore::math::Vec3 b) {
    const auto pa = projectMeshPreview(bounds, a);
    const auto pb = projectMeshPreview(bounds, b);
    frame.debugLines.push_back(novacore::render::DebugLine{
        pa[0],
        pa[1],
        pb[0],
        pb[1],
        {0.20F, 0.80F, 0.92F, 0.62F},
    });
}

[[nodiscard]] bool actionPressed(
    const novacore::platform::InputActionMap& actions,
    std::string_view action) {
    return actions.stateOrDefault(action).pressed;
}

} // namespace

GameApp::GameApp(GameAppOptions options)
    : options_(options) {}

void GameApp::onStartup() {
    actions_ = input::createDefaultActionMap();
    attachmentRegistry_.registerPrototypeAttachments();
    activeLoadout_ = weapons::defaultPrototypeLoadout(std::string(kDefaultWeaponId));
    if (options_.autoEnterDevRange) {
        menu_.showDevRange();
    }

    const auto watchConfig = [this](std::string name, std::string path) {
        const auto event = configRegistry_.watchJson(name, path);
        if (!event.loaded) {
            novacore::core::logWarning("game", "Initial config load failed: " + name);
        }
    };
    watchConfig("input", "configs/input/default_input.json");
    watchConfig("movement", "configs/movement/player_movement.json");
    watchConfig("render", "configs/render/dev_range_render.json");
    watchConfig("weapons", "configs/weapons/core_trio.json");
    watchConfig("modes", "configs/game_modes/tdm_control.json");

    applyLoadedConfigs();
    loadUserSettings();
    loadAssetCatalog();

    novacore::platform::WindowDesc windowDesc{};
    windowDesc.title = menu_.title();
    windowDesc.width = 1280;
    windowDesc.height = 720;
    windowDesc.preferVulkan = options_.preferVulkanRenderer;

    window_.create(windowDesc);

    novacore::render::RendererCreateInfo rendererInfo{};
    rendererInfo.preferVulkan = options_.preferVulkanRenderer;
    rendererInfo.requireVulkan = options_.requireVulkanRenderer;
    renderer_.create(window_, rendererInfo);
    registerDevMeshResources();

    cameraEntity_ = world_.createEntity();
    world_.addComponent(cameraEntity_, novacore::ecs::NameComponent{"main_camera"});
    world_.addComponent(cameraEntity_, novacore::ecs::TransformComponent{});
    world_.addComponent(cameraEntity_, novacore::ecs::CameraComponent{});

    if (weapons_.weaponCount() == 0) {
        weapons_.registerPrototypeLoadout();
    }
    rebuildActiveAttachmentSummary();
    ensureLocalPlayer();
    syncRuntimeLoadout();

    novacore::core::logInfo("game", "Nemisis sandbox camera entity created");
    novacore::core::logInfo("game", "Prototype weapon registry initialized");
    novacore::core::logInfo("game", "Default input action map and config registry initialized");
    novacore::core::logInfo(
        "game",
        "Launch profile: renderer=" +
            std::string(options_.preferVulkanRenderer ? "vulkan" : "sdl-debug") +
            " require_vulkan=" + std::string(options_.requireVulkanRenderer ? "true" : "false") +
            " start_screen=" + std::string(options_.autoEnterDevRange ? "dev_range" : "menu") +
            " lock_dev_range=" + std::string(options_.lockDevRange ? "true" : "false"));

    if (window_.isHeadless()) {
        novacore::core::logInfo("game", "Running in HEADLESS mode (no SDL3/Vulkan). Press Ctrl+C in your terminal/console to exit.");
    }
}

void GameApp::onShutdown() {
    releaseDevMeshResources();
    renderer_.shutdown();
    window_.shutdown();
}

void GameApp::onFixedTick(const novacore::core::FrameContext& context) {
    if (!menu_.gameplayActive()) {
        return;
    }

    ensureLocalPlayer();

    syncRuntimeLoadout();

    const auto command = input::buildPlayerInputCommand(actions_, context.tickIndex, settings_);
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
    dev::GreyboxCollisionResult collisionSample{};
    if (movementState != nullptr) {
        *movementState = movement_.simulate(*movementState, movementCommand, static_cast<float>(context.fixedDeltaSeconds));
        collisionSample = dev::resolveGreyboxPlayerCollision(
            greyboxWorld_,
            dev::GreyboxCollisionQuery{movementState->position, 0.42F, 1.80F});
        if (collisionSample.blocked) {
            if (std::abs(collisionSample.correction.x) > 0.0001F) {
                movementState->velocity.x = 0.0F;
            }
            if (std::abs(collisionSample.correction.z) > 0.0001F) {
                movementState->velocity.z = 0.0F;
            }
        }
        if (collisionSample.grounded && movementState->velocity.y < 0.0F) {
            movementState->velocity.y = 0.0F;
        }
        if (collisionSample.grounded) {
            movementState->hasDoubleJump = true;
            if (movementState->mode == movement::MovementMode::Airborne ||
                movementState->mode == movement::MovementMode::Dashing ||
                movementState->mode == movement::MovementMode::Mantling) {
                movementState->mode = movement::MovementMode::Grounded;
            }
        }
        movementState->position = collisionSample.position;
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
    if (weaponState != nullptr) {
        ensureActiveWeapon(*weaponState, activeAttachmentBuild_.effectiveWeapon);
    }

    tickRangeSession(static_cast<float>(context.fixedDeltaSeconds));

    weapons::FireResult fireResult{};
    weapons::ShotTraceResult shotTrace{};
    bool hasShotTrace = false;
    const float movementSpeed = movementState != nullptr
        ? std::sqrt((movementState->velocity.x * movementState->velocity.x) +
                    (movementState->velocity.z * movementState->velocity.z))
        : 0.0F;
    const bool airborne = movementState != nullptr && movementState->mode == movement::MovementMode::Airborne;
    const bool sprinting = command.sprintHeld || command.tacticalSprintHeld;
    if (weaponState != nullptr) {
        const auto& weapon = activeAttachmentBuild_.effectiveWeapon;
        if (!weapon.id.empty()) {
            weapons::FireRequest fireRequest{};
            fireRequest.triggerHeld = command.fireHeld;
            fireRequest.reloadPressed = command.reloadPressed;
            fireRequest.adsHeld = command.adsHeld;
            fireRequest.movementSpeed = movementSpeed;
            fireRequest.airborne = airborne;
            fireRequest.sprinting = sprinting;
            fireRequest.fixedDeltaSeconds = static_cast<float>(context.fixedDeltaSeconds);
            fireResult = weapons::simulateWeaponTick(weapon, *weaponState, fireRequest);
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
                shotRequest.movementSpeed = movementSpeed;
                shotRequest.adsAlpha = fireResult.adsAlpha;
                shotRequest.recoilPitchDegrees = fireResult.recoilPitchOffsetDegrees;
                shotRequest.recoilYawDegrees = fireResult.recoilYawOffsetDegrees;
                shotRequest.ads = command.adsHeld;
                shotRequest.airborne = airborne;
                shotRequest.sprinting = sprinting;
                shotTrace = weapons::buildShotTrace(weapon, shotRequest);
                hasShotTrace = true;
            }
        }
    }

    dev::DebugTargetHitResult targetHit{};
    dev::DevTargetRangeHitResult targetRangeHit{};
    if (hasShotTrace) {
        targetRangeHit = dev::applyShotToDevTargetRange(targetRange_, shotTrace);
        targetHit = targetRangeHit.targetHit;
    }
    dev::recordShotResult(devRangeSession_, fireResult, targetHit, devRangeTuning_);
    if (targetRangeHit.hit && !targetRangeHit.laneName.empty() && !devRangeSession_.eventText.empty()) {
        devRangeSession_.eventText += "  " + targetRangeHit.laneName;
    }
    if (targetHit.eliminated) {
        dev::beginTargetRespawn(devRangeSession_, devRangeTuning_);
        dev::beginTargetLaneRespawn(targetRange_, targetRangeHit.laneIndex, devRangeTuning_.targetRespawnDelaySeconds);
    }

    (void)dev::tickDevTargetRangeRespawns(targetRange_, static_cast<float>(context.fixedDeltaSeconds));
    devRangeSession_.targetRespawnSeconds = dev::nextTargetRespawnSeconds(targetRange_);

    player::PlayerHealthComponent healthSample{};
    if (auto* health = world_.getComponent<player::PlayerHealthComponent>(localPlayerEntity_);
        health != nullptr) {
        dev::tickPlayerRegen(devRangeSession_, *health, static_cast<float>(context.fixedDeltaSeconds), devRangeTuning_);
        if (dev::tickPlayerRespawn(devRangeSession_, *health, static_cast<float>(context.fixedDeltaSeconds))) {
            resetDevRangeState();
        }
        healthSample = *health;
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

    if (movementState != nullptr && view != nullptr) {
        const auto cameraFrame = player::updateCameraRig(
            cameraRig_,
            player::CameraRigInput{
                movementState->position,
                movementState->velocity,
                movementState->mode,
                *view,
                weaponSample,
                command.adsHeld,
                command.sprintHeld,
                command.tacticalSprintHeld,
                command.crouchHeld,
                static_cast<float>(context.fixedDeltaSeconds),
            });
        if (auto* cameraTransform = world_.getComponent<novacore::ecs::TransformComponent>(cameraEntity_);
            cameraTransform != nullptr) {
            cameraTransform->position = cameraFrame.position;
        }
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
        healthSample,
        devRangeSession_,
        targetRange_,
        dev::activeTarget(targetRange_) != nullptr ? *dev::activeTarget(targetRange_) : dev::DebugTargetState{},
        targetHit,
        networkSample,
        loopbackBridge_.stats(),
        view != nullptr ? *view : player::PlayerViewComponent{},
        collisionSample,
    });
}

void GameApp::onFrame(const novacore::core::FrameContext& context) {
    window_.pollEvents(input_);
    actions_.update(window_.inputSnapshot());
    menu_.update(actions_, settings_, activeLoadout_, attachmentRegistry_);
    menu_.updateFrame(context.deltaSeconds);
    syncRuntimeLoadout();
    persistUserSettingsIfChanged();
    if (menu_.gameplayActive() && actionPressed(actions_, input::actions::ResetRange)) {
        resetDevRangeState();
    }
    if (options_.lockDevRange && !menu_.gameplayActive()) {
        menu_.showDevRange();
    }
    advanceMenuFlowSmoke(context);
    syncRelativeMouseMode();
    window_.setTitle(menu_.title());

    for (const auto& event : configRegistry_.pollReloads()) {
        if (event.loaded) {
            applyConfig(event.name);
            novacore::core::logInfo("game", "Reloaded config: " + event.name);
        } else {
            novacore::core::logWarning("game", "Config reload failed: " + event.name);
        }
    }

    if (menu_.gameplayActive()) {
        devSandbox_.onFrame(context.deltaSeconds);
    }

    const auto meshStats = renderer_.meshResourceStats();
    novacore::render::RenderFrameInfo frameInfo{};
    frameInfo.clearColor = menu_.gameplayActive() ? devSandbox_.clearColor() : menu_.clearColor();
    if (menu_.gameplayActive()) {
        latestDevRangeRenderStats_ = devRangeRenderer_.append(
            frameInfo,
            dev::DevRangeRenderSceneDesc{
                &greyboxWorld_,
                &targetRange_,
                &devSandbox_.latestSample().collision,
                &devMeshResources_,
                currentPlayerRenderState(),
                renderTuning_.lighting,
                renderTuning_.showWorldDebugLines && settings_.video.showDebugWorldLines,
                renderTuning_.verticalFovDegrees,
                renderTuning_.nearPlane,
                renderTuning_.farPlane,
            });
    } else {
        latestDevRangeRenderStats_ = {};
    }
    menu_.appendRenderCommands(
        frameInfo,
        devSandbox_.latestSample(),
        greyboxWorld_,
        renderer_.backendName(),
        renderer_.vulkanSummary(),
        assetStreamer_.pendingCount(),
        devAssetSummary_,
        meshStats,
        renderer_.backendFrameStats(),
        latestDevRangeRenderStats_,
        settings_,
        activeLoadout_,
        attachmentRegistry_,
        activeAttachmentBuild_,
        accountStats_);
    appendA0MeshWireframePreview(frameInfo);
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
    devAssetSummary_ = devAssetBindings_.bindRequiredAssets(assetCatalog_, ".");
    novacore::core::logInfo(
        "game",
        "Asset catalog loaded: " + std::to_string(assetCatalog_.assetCount()) + " assets");
    novacore::core::logInfo(
        "game",
        "Dev sandbox asset preload requests queued: " + std::to_string(assetStreamer_.pendingCount()));
    novacore::core::logInfo(
        "game",
        "Dev mesh assets ready: " + std::to_string(devAssetSummary_.renderableAssetCount) + "/" +
            std::to_string(devAssetSummary_.requiredAssetCount) +
            " metadata=" + std::to_string(devAssetSummary_.metadataAssetCount) +
            " imported=" + std::to_string(devAssetSummary_.importedAssetCount) +
            " meshes=" + std::to_string(devAssetSummary_.totalMeshCount) +
            " nodes=" + std::to_string(devAssetSummary_.totalNodeCount) +
            " materials=" + std::to_string(devAssetSummary_.totalMaterialCount) +
            " primitives=" + std::to_string(devAssetSummary_.totalPrimitiveCount) +
            " vertices=" + std::to_string(devAssetSummary_.totalVertexCount) +
            " indices=" + std::to_string(devAssetSummary_.totalIndexCount));
    for (const auto& error : devAssetSummary_.errors) {
        novacore::core::logWarning("game", "Dev asset binding failed: " + error);
    }
}

void GameApp::applyConfig(std::string_view name) {
    if (name == "input") {
        const auto* document = configRegistry_.find("input");
        if (document != nullptr) {
            settings_ = settings::gameSettingsFromConfig(*document, settings_);
        }
        return;
    }

    if (name == "movement") {
        const auto* document = configRegistry_.find("movement");
        if (document != nullptr) {
            movement_.setTuning(movement::movementTuningFromConfig(*document, movement_.tuning()));
        }
        return;
    }

    if (name == "render") {
        const auto* document = configRegistry_.find("render");
        if (document != nullptr) {
            renderTuning_ = render::devRenderTuningFromConfig(*document, renderTuning_);
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
            if (weaponState != nullptr) {
                rebuildActiveAttachmentSummary();
                ensureActiveWeapon(*weaponState, activeAttachmentBuild_.effectiveWeapon);
            }
        }
    }
}

void GameApp::applyLoadedConfigs() {
    applyConfig("input");
    applyConfig("movement");
    applyConfig("render");
    applyConfig("weapons");
}

void GameApp::loadUserSettings() {
    settings::UserSettingsSnapshot fallback{};
    fallback.settings = settings_;
    fallback.loadout = activeLoadout_;

    const auto result = settings::loadUserSettingsSnapshot(userSettingsPath_, fallback, attachmentRegistry_);
    if (!result.ok()) {
        for (const auto& error : result.errors) {
            novacore::core::logWarning("game", error);
        }
        lastPersistedUserSettings_ = settings::serializeUserSettingsSnapshot(fallback);
        return;
    }

    if (result.loaded) {
        settings_ = result.snapshot.settings;
        activeLoadout_ = result.snapshot.loadout;
        novacore::core::logInfo("game", "User settings loaded: " + userSettingsPath_.string());
    }
    lastPersistedUserSettings_ = settings::serializeUserSettingsSnapshot(settings::UserSettingsSnapshot{settings_, activeLoadout_});
}

void GameApp::persistUserSettingsIfChanged() {
    const settings::UserSettingsSnapshot snapshot{settings_, activeLoadout_};
    const auto serialized = settings::serializeUserSettingsSnapshot(snapshot);
    if (serialized == lastPersistedUserSettings_) {
        return;
    }

    const auto result = settings::saveUserSettingsSnapshot(userSettingsPath_, snapshot);
    if (!result.ok()) {
        for (const auto& error : result.errors) {
            novacore::core::logWarning("game", error);
        }
        return;
    }

    lastPersistedUserSettings_ = serialized;
    if (result.saved) {
        novacore::core::logInfo("game", "User settings saved: " + userSettingsPath_.string());
    }
}

void GameApp::rebuildActiveAttachmentSummary() {
    if (activeLoadout_.weaponId.empty() || weapons_.findWeapon(activeLoadout_.weaponId) == nullptr) {
        activeLoadout_.weaponId = std::string(kDefaultWeaponId);
    }

    const auto* baseWeapon = weapons_.findWeapon(activeLoadout_.weaponId);
    if (baseWeapon == nullptr) {
        baseWeapon = weapons_.findWeapon(kDefaultWeaponId);
    }
    if (baseWeapon == nullptr) {
        activeAttachmentBuild_ = {};
        return;
    }

    activeLoadout_.weaponId = baseWeapon->id;
    activeAttachmentBuild_ = weapons::buildWeaponWithAttachments(*baseWeapon, activeLoadout_, attachmentRegistry_);
}

void GameApp::syncRuntimeLoadout() {
    rebuildActiveAttachmentSummary();

    if (localPlayerEntity_.isNull() || !world_.isAlive(localPlayerEntity_)) {
        return;
    }

    if (auto* loadout = world_.getComponent<player::PlayerLoadoutComponent>(localPlayerEntity_);
        loadout != nullptr) {
        loadout->activeWeaponId = activeLoadout_.weaponId;
    }

    if (auto* weaponState = world_.getComponent<weapons::WeaponRuntimeState>(localPlayerEntity_);
        weaponState != nullptr && !activeAttachmentBuild_.effectiveWeapon.id.empty()) {
        ensureActiveWeapon(*weaponState, activeAttachmentBuild_.effectiveWeapon);
    }
}

void GameApp::ensureActiveWeapon(
    weapons::WeaponRuntimeState& weaponState,
    const weapons::WeaponDefinition& effectiveWeapon) {
    if (effectiveWeapon.id.empty()) {
        return;
    }

    if (weaponState.weaponId != effectiveWeapon.id) {
        weaponState = weapons::WeaponRuntimeState{};
        weaponState.weaponId = effectiveWeapon.id;
        weaponState.ammoInMagazine = effectiveWeapon.magazineSize;
        return;
    }

    weaponState.ammoInMagazine = std::min(weaponState.ammoInMagazine, effectiveWeapon.magazineSize);
    weaponState.reloadTimeRemaining = std::min(weaponState.reloadTimeRemaining, effectiveWeapon.reloadTimeSeconds);
}

void GameApp::ensureLocalPlayer() {
    if (!localPlayerEntity_.isNull() && world_.isAlive(localPlayerEntity_)) {
        return;
    }

    player::LocalPlayerSpawnDesc spawnDesc{};
    spawnDesc.activeWeaponId = activeLoadout_.weaponId.empty()
        ? std::string(kDefaultWeaponId)
        : activeLoadout_.weaponId;
    spawnDesc.position = greyboxWorld_.playerSpawn;
    rebuildActiveAttachmentSummary();
    localPlayerEntity_ = player::spawnLocalPlayer(world_, spawnDesc, &activeAttachmentBuild_.effectiveWeapon);
    localCommandQueue_.clear();
    player::resetCameraRig(cameraRig_);
    novacore::core::logInfo("game", "Local player entity spawned");
}

void GameApp::resetDevRangeState() {
    ensureLocalPlayer();
    rebuildActiveAttachmentSummary();
    dev::resetDevTargetRange(targetRange_);
    dev::recordRangeReset(devRangeSession_, devRangeTuning_);

    if (auto* movementState = world_.getComponent<movement::PlayerMovementState>(localPlayerEntity_);
        movementState != nullptr) {
        *movementState = {};
        movementState->position = greyboxWorld_.playerSpawn;
    }
    if (auto* transform = world_.getComponent<novacore::ecs::TransformComponent>(localPlayerEntity_);
        transform != nullptr) {
        transform->position = greyboxWorld_.playerSpawn;
    }
    if (auto* cameraTransform = world_.getComponent<novacore::ecs::TransformComponent>(cameraEntity_);
        cameraTransform != nullptr) {
        cameraTransform->position = greyboxWorld_.playerSpawn + novacore::math::Vec3{0.0F, 1.65F, 0.0F};
    }
    if (auto* view = world_.getComponent<player::PlayerViewComponent>(localPlayerEntity_);
        view != nullptr) {
        *view = {};
    }
    if (auto* health = world_.getComponent<player::PlayerHealthComponent>(localPlayerEntity_);
        health != nullptr) {
        player::resetHealth(*health);
    }
    if (auto* weapon = world_.getComponent<weapons::WeaponRuntimeState>(localPlayerEntity_);
        weapon != nullptr) {
        *weapon = {};
        ensureActiveWeapon(*weapon, activeAttachmentBuild_.effectiveWeapon);
    }

    localCommandQueue_.clear();
    player::resetCameraRig(cameraRig_);
}

void GameApp::tickRangeSession(float fixedDeltaSeconds) {
    dev::tickSessionFeedback(devRangeSession_, fixedDeltaSeconds);
}

void GameApp::advanceMenuFlowSmoke(const novacore::core::FrameContext& context) {
    if (!options_.runMenuFlowSmoke) {
        return;
    }

    const auto frame = context.frameIndex;
    std::size_t stage = 0;
    if (frame < 4U) {
        stage = 0;
    } else if (frame < 8U) {
        stage = 1;
    } else if (frame < 12U) {
        stage = 2;
    } else if (frame < 16U) {
        stage = 3;
    } else if (frame < 20U) {
        stage = 4;
    } else if (frame < 28U) {
        stage = 5;
    } else if (frame < 32U) {
        stage = 6;
    } else if (frame < 36U) {
        stage = 7;
    } else if (frame < 40U) {
        stage = 8;
    } else {
        stage = 9;
    }

    if (stage == menuFlowSmokeStage_) {
        return;
    }
    menuFlowSmokeStage_ = stage;

    std::string_view label = "unknown";
    switch (stage) {
    case 0:
        menu_.showMainMenu(ui::MenuTab::Play);
        label = "main_menu/play";
        break;
    case 1:
        menu_.showMainMenu(ui::MenuTab::Loadout);
        label = "main_menu/loadout";
        break;
    case 2:
        menu_.showMainMenu(ui::MenuTab::Settings);
        label = "main_menu/settings";
        break;
    case 3:
        menu_.showMainMenu(ui::MenuTab::Account);
        label = "main_menu/account";
        break;
    case 4:
        menu_.showLoadingScreen(ui::GameScreen::DevRange, 0.58F);
        label = "loading/dev_range";
        break;
    case 5:
        menu_.showDevRange();
        resetDevRangeState();
        label = "dev_range";
        break;
    case 6:
        menu_.showLoadingScreen(ui::GameScreen::TeamDeathmatch, 0.64F);
        label = "loading/team_deathmatch";
        break;
    case 7:
        menu_.showTeamDeathmatch();
        label = "team_deathmatch";
        break;
    case 8:
        menu_.showLoadingScreen(ui::GameScreen::Control, 0.70F);
        label = "loading/control";
        break;
    case 9:
        menu_.showControl();
        label = "control";
        break;
    default:
        break;
    }

    novacore::core::logInfo("game", "Menu flow smoke stage: " + std::string(label));
}

void GameApp::syncRelativeMouseMode() {
    const bool wantsRelativeMouse = menu_.gameplayActive() && !window_.isHeadless();
    if (relativeMouseDesired_ == wantsRelativeMouse) {
        return;
    }

    relativeMouseDesired_ = wantsRelativeMouse;
    if (!window_.setRelativeMouseMode(wantsRelativeMouse) && wantsRelativeMouse) {
        novacore::core::logWarning("game", "Relative mouse mode requested but unavailable on this platform backend");
    }
}

void GameApp::registerDevMeshResources() {
    devMeshResources_.clear();

    std::size_t missing = 0;
    std::size_t rejected = 0;
    for (const auto assetId : assets::requiredDevSandboxRenderableAssetIds()) {
        const auto* source = devAssetBindings_.meshCatalog().findByAssetId(assetId);
        if (source == nullptr || !source->meshData.has_value()) {
            ++missing;
            novacore::core::logWarning("game", "Dev mesh resource missing CPU mesh data: " + std::string(assetId));
            continue;
        }

        const auto handle = renderer_.registerMeshResource(std::string(assetId), *source->meshData);
        if (!handle.isValid()) {
            ++rejected;
            novacore::core::logWarning("game", "Dev mesh resource registration failed: " + std::string(assetId));
            continue;
        }
        devMeshResources_[std::string(assetId)] = handle;
    }

    const auto stats = renderer_.meshResourceStats();
    novacore::core::logInfo(
        "game",
        "Renderer dev mesh resources registered: " + std::to_string(devMeshResources_.size()) + "/" +
            std::to_string(assets::requiredDevSandboxRenderableAssetIds().size()) +
            " cpu_registered=" + std::to_string(stats.registeredResources) +
            " primitives=" + std::to_string(stats.totalPrimitives) +
            " vertices=" + std::to_string(stats.totalVertices) +
            " indices=" + std::to_string(stats.totalIndices) +
            " missing=" + std::to_string(missing) +
            " rejected=" + std::to_string(rejected));
}

void GameApp::releaseDevMeshResources() {
    for (const auto& [_, handle] : devMeshResources_) {
        renderer_.releaseMeshResource(handle);
    }
    devMeshResources_.clear();
}

void GameApp::appendA0MeshWireframePreview(novacore::render::RenderFrameInfo& frame) const {
    if (!menu_.gameplayActive()) {
        return;
    }

    const auto* arena = devAssetBindings_.meshCatalog().findByAssetId("env_test_arena_kit_01");
    if (arena == nullptr || !arena->meshData.has_value()) {
        return;
    }

    MeshPreviewBounds bounds{};
    for (const auto& primitive : arena->meshData->primitives) {
        for (const auto& position : primitive.positions) {
            bounds.include(position);
        }
    }
    if (!bounds.valid()) {
        return;
    }

    frame.debugTexts.push_back(novacore::render::DebugText{
        370.0F,
        206.0F,
        1.0F,
        {0.45F, 0.92F, 0.98F, 1.0F},
        "A0 GLB WIREFRAME",
    });

    constexpr std::size_t kLineBudget = 2400;
    std::size_t emittedLines = 0;
    for (const auto& primitive : arena->meshData->primitives) {
        for (std::size_t index = 0; index + 2 < primitive.indices.size(); index += 3) {
            const auto ia = static_cast<std::size_t>(primitive.indices[index]);
            const auto ib = static_cast<std::size_t>(primitive.indices[index + 1]);
            const auto ic = static_cast<std::size_t>(primitive.indices[index + 2]);
            if (ia >= primitive.positions.size() ||
                ib >= primitive.positions.size() ||
                ic >= primitive.positions.size()) {
                continue;
            }

            addPreviewLine(frame, bounds, primitive.positions[ia], primitive.positions[ib]);
            addPreviewLine(frame, bounds, primitive.positions[ib], primitive.positions[ic]);
            addPreviewLine(frame, bounds, primitive.positions[ic], primitive.positions[ia]);
            emittedLines += 3;
            if (emittedLines >= kLineBudget) {
                return;
            }
        }
    }
}

dev::DevRangePlayerRenderState GameApp::currentPlayerRenderState() const {
    dev::DevRangePlayerRenderState state{};
    state.position = greyboxWorld_.playerSpawn;
    if (const auto* movementState = world_.getComponent<movement::PlayerMovementState>(localPlayerEntity_);
        movementState != nullptr) {
        state.position = movementState->position;
        state.hasMovementState = true;
    } else if (const auto* transform = world_.getComponent<novacore::ecs::TransformComponent>(localPlayerEntity_);
        transform != nullptr) {
        state.position = transform->position;
    }

    if (const auto* playerView = world_.getComponent<player::PlayerViewComponent>(localPlayerEntity_);
        playerView != nullptr) {
        state.view = *playerView;
    }

    if (cameraRig_.initialized) {
        state.cameraPosition = cameraRig_.position;
        state.cameraView = cameraRig_.view;
        state.headBobOffset = cameraRig_.headBobOffset;
        state.weaponSwayOffset = cameraRig_.weaponSwayOffset;
        state.cameraRollDegrees = cameraRig_.rollDegrees;
        state.verticalFovDegrees = cameraRig_.verticalFovDegrees;
        state.speed01 = cameraRig_.speed01;
        state.adsAlpha = cameraRig_.adsAlpha;
        state.hasCameraRig = true;
    }

    state.activeWeaponId = activeAttachmentBuild_.effectiveWeapon.id;
    state.activeWeaponClass = activeAttachmentBuild_.effectiveWeapon.weaponClass;
    state.effectiveMagazineSize = activeAttachmentBuild_.effectiveMagazineSize;
    if (const auto* weapon = world_.getComponent<weapons::WeaponRuntimeState>(localPlayerEntity_);
        weapon != nullptr) {
        state.weapon = *weapon;
    }

    return state;
}

} // namespace nemisis::game
