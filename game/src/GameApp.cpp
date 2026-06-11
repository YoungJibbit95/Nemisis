#include "nemisis/GameApp.hpp"

#include "nemisis/dev/DebugTarget.hpp"
#include "nemisis/dev/GreyboxCollision.hpp"
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
#include <limits>
#include <string>
#include <string_view>

namespace nemisis::game {

namespace {

constexpr std::string_view kDefaultWeaponId = "ar_01";
constexpr float kDebugTargetRespawnDelaySeconds = 1.5F;

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

} // namespace

GameApp::GameApp(GameAppOptions options)
    : options_(options) {}

void GameApp::onStartup() {
    actions_ = input::createDefaultActionMap();
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
    watchConfig("weapons", "configs/weapons/core_trio.json");
    watchConfig("modes", "configs/game_modes/tdm_control.json");

    applyLoadedConfigs();
    loadAssetCatalog();

    novacore::platform::WindowDesc windowDesc{};
    windowDesc.title = menu_.title();
    windowDesc.width = 1280;
    windowDesc.height = 720;
    windowDesc.preferVulkan = options_.preferVulkanRenderer;

    window_.create(windowDesc);

    novacore::render::RendererCreateInfo rendererInfo{};
    rendererInfo.preferVulkan = options_.preferVulkanRenderer;
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
    if (!menu_.gameplayActive()) {
        return;
    }

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
        view != nullptr ? *view : player::PlayerViewComponent{},
        collisionSample,
    });
}

void GameApp::onFrame(const novacore::core::FrameContext& context) {
    window_.pollEvents(input_);
    actions_.update(window_.inputSnapshot());
    menu_.update(actions_);
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

    novacore::render::RenderFrameInfo frameInfo{};
    frameInfo.clearColor = menu_.gameplayActive() ? devSandbox_.clearColor() : menu_.clearColor();
    menu_.appendRenderCommands(
        frameInfo,
        devSandbox_.latestSample(),
        greyboxWorld_,
        renderer_.backendName(),
        renderer_.vulkanSummary(),
        assetStreamer_.pendingCount(),
        devAssetSummary_);
    appendA0MeshWireframePreview(frameInfo);
    appendGreyboxWorld3D(frameInfo);
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
    spawnDesc.position = greyboxWorld_.playerSpawn;
    localPlayerEntity_ = player::spawnLocalPlayer(world_, spawnDesc, weapons_.findWeapon(spawnDesc.activeWeaponId));
    localCommandQueue_.clear();
    novacore::core::logInfo("game", "Local player entity spawned");
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

const novacore::assets::GltfMeshData* GameApp::findDevMeshData(std::string_view assetId) const {
    const auto* source = devAssetBindings_.meshCatalog().findByAssetId(assetId);
    if (source == nullptr || !source->meshData.has_value()) {
        return nullptr;
    }
    return &(*source->meshData);
}

void GameApp::appendDevMeshInstance(
    novacore::render::RenderFrameInfo& frame,
    std::string_view assetId,
    novacore::math::Vec3 position,
    novacore::math::Vec3 scale,
    float yawDegrees,
    std::array<float, 4> color) const {
    const auto* meshData = findDevMeshData(assetId);
    if (meshData == nullptr) {
        return;
    }

    frame.worldMeshes.push_back(novacore::render::RenderMesh3D{
        std::string(assetId),
        meshData,
        position,
        scale,
        yawDegrees,
        color,
    });
}

void GameApp::appendGreyboxWorld3D(novacore::render::RenderFrameInfo& frame) const {
    if (!menu_.gameplayActive()) {
        return;
    }

    novacore::math::Vec3 playerPosition = greyboxWorld_.playerSpawn;
    if (const auto* movementState = world_.getComponent<movement::PlayerMovementState>(localPlayerEntity_);
        movementState != nullptr) {
        playerPosition = movementState->position;
    } else if (const auto* transform = world_.getComponent<novacore::ecs::TransformComponent>(localPlayerEntity_);
        transform != nullptr) {
        playerPosition = transform->position;
    }

    player::PlayerViewComponent view{};
    if (const auto* playerView = world_.getComponent<player::PlayerViewComponent>(localPlayerEntity_);
        playerView != nullptr) {
        view = *playerView;
    }

    frame.camera3D.enabled = true;
    frame.camera3D.position = playerPosition + novacore::math::Vec3{0.0F, 1.65F, 0.0F};
    frame.camera3D.yawDegrees = view.yawDegrees;
    frame.camera3D.pitchDegrees = view.pitchDegrees;
    frame.camera3D.verticalFovDegrees = 74.0F;
    frame.camera3D.nearPlane = 0.03F;
    frame.camera3D.farPlane = 120.0F;

    frame.worldBoxes.reserve(frame.worldBoxes.size() + greyboxWorld_.primitives.size() + 3U);
    for (const auto& primitive : greyboxWorld_.primitives) {
        auto color = primitive.color;
        if (primitive.kind == dev::GreyboxPrimitiveKind::Target && debugTarget_.eliminated) {
            color = {0.10F, 0.10F, 0.10F, 1.0F};
        }
        frame.worldBoxes.push_back(novacore::render::RenderBox3D{
            primitive.center,
            primitive.halfExtents,
            color,
        });
    }

    const auto vectors = player::viewVectors(view);
    frame.worldMeshes.reserve(frame.worldMeshes.size() + 12U);
    appendDevMeshInstance(
        frame,
        "env_test_arena_kit_01",
        novacore::math::Vec3{0.0F, 0.0F, 0.0F},
        novacore::math::Vec3{1.0F, 1.0F, 1.0F},
        0.0F,
        {0.46F, 0.54F, 0.55F, 1.0F});
    appendDevMeshInstance(
        frame,
        debugTarget_.eliminated ? "prop_target_dummy_01" : "chr_dev_soldier_a",
        novacore::math::Vec3{0.0F, 0.0F, 15.0F},
        novacore::math::Vec3{1.0F, 1.0F, 1.0F},
        180.0F,
        debugTarget_.eliminated
            ? std::array<float, 4>{0.20F, 0.20F, 0.20F, 1.0F}
            : std::array<float, 4>{0.78F, 0.26F, 0.20F, 1.0F});
    appendDevMeshInstance(
        frame,
        "prop_target_dummy_01",
        novacore::math::Vec3{-5.5F, 0.0F, 18.0F},
        novacore::math::Vec3{0.85F, 0.85F, 0.85F},
        180.0F,
        {0.90F, 0.42F, 0.18F, 1.0F});
    appendDevMeshInstance(
        frame,
        "prop_target_dummy_01",
        novacore::math::Vec3{5.5F, 0.0F, 18.0F},
        novacore::math::Vec3{0.85F, 0.85F, 0.85F},
        180.0F,
        {0.90F, 0.42F, 0.18F, 1.0F});
    appendDevMeshInstance(
        frame,
        "chr_proto_humanoid_01",
        novacore::math::Vec3{-3.2F, 0.0F, 14.0F},
        novacore::math::Vec3{1.0F, 1.0F, 1.0F},
        180.0F,
        {0.28F, 0.74F, 0.90F, 1.0F});
    appendDevMeshInstance(
        frame,
        "map_floor_tile_01",
        novacore::math::Vec3{-7.0F, 0.0F, -4.0F},
        novacore::math::Vec3{1.0F, 1.0F, 1.0F},
        0.0F,
        {0.32F, 0.38F, 0.40F, 1.0F});
    appendDevMeshInstance(
        frame,
        "map_wall_panel_01",
        novacore::math::Vec3{-10.5F, 0.0F, 10.0F},
        novacore::math::Vec3{1.0F, 1.0F, 1.0F},
        90.0F,
        {0.36F, 0.44F, 0.47F, 1.0F});
    appendDevMeshInstance(
        frame,
        "map_cover_crate_01",
        novacore::math::Vec3{-4.2F, 0.0F, 2.0F},
        novacore::math::Vec3{1.0F, 1.0F, 1.0F},
        0.0F,
        {0.42F, 0.48F, 0.45F, 1.0F});
    appendDevMeshInstance(
        frame,
        "map_ramp_01",
        novacore::math::Vec3{4.0F, 0.0F, -2.5F},
        novacore::math::Vec3{1.0F, 1.0F, 1.0F},
        0.0F,
        {0.30F, 0.47F, 0.43F, 1.0F});
    appendDevMeshInstance(
        frame,
        "map_target_stand_01",
        novacore::math::Vec3{3.2F, 0.0F, 14.0F},
        novacore::math::Vec3{1.0F, 1.0F, 1.0F},
        180.0F,
        {0.88F, 0.38F, 0.16F, 1.0F});

    const auto weaponCenter =
        frame.camera3D.position +
        (vectors.forward * 0.92F) +
        (vectors.horizontalRight * 0.28F) +
        novacore::math::Vec3{0.0F, -0.24F, 0.0F};
    frame.worldBoxes.push_back(novacore::render::RenderBox3D{
        weaponCenter,
        novacore::math::Vec3{0.18F, 0.08F, 0.42F},
        {0.08F, 0.11F, 0.12F, 1.0F},
    });
    appendDevMeshInstance(
        frame,
        "wpn_ar_01",
        weaponCenter,
        novacore::math::Vec3{0.35F, 0.35F, 0.35F},
        view.yawDegrees,
        {0.16F, 0.18F, 0.19F, 1.0F});
    appendDevMeshInstance(
        frame,
        "wpn_proto_smg_01",
        frame.camera3D.position +
            (vectors.forward * 1.10F) +
            (vectors.horizontalRight * -0.18F) +
            novacore::math::Vec3{0.0F, -0.30F, 0.0F},
        novacore::math::Vec3{0.42F, 0.42F, 0.42F},
        view.yawDegrees + 90.0F,
        {0.10F, 0.16F, 0.18F, 1.0F});
}

} // namespace nemisis::game
