#pragma once

#include "nemisis/assets/DevAssetBindings.hpp"
#include "nemisis/assets/GameAssetCatalog.hpp"
#include "nemisis/dev/DevRangeSession.hpp"
#include "nemisis/dev/DevRangeRigidBodies.hpp"
#include "nemisis/dev/DevRangeRenderScene.hpp"
#include "nemisis/dev/DevSandbox.hpp"
#include "nemisis/dev/DevTargetRange.hpp"
#include "nemisis/dev/GreyboxWorld.hpp"
#include "nemisis/input/GameplayInputBuffer.hpp"
#include "nemisis/movement/MovementConfig.hpp"
#include "nemisis/movement/MovementSystem.hpp"
#include "nemisis/net/LoopbackCommandBridge.hpp"
#include "nemisis/net/PredictionHistory.hpp"
#include "nemisis/net/SnapshotInterpolation.hpp"
#include "nemisis/player/PlayerAnimation.hpp"
#include "nemisis/player/PlayerCameraRig.hpp"
#include "nemisis/player/PlayerCommandQueue.hpp"
#include "nemisis/player/PlayerProfile.hpp"
#include "nemisis/player/PlayerSkeletalAnimator.hpp"
#include "nemisis/render/RenderTuning.hpp"
#include "nemisis/settings/GameSettings.hpp"
#include "nemisis/settings/UserSettingsPersistence.hpp"
#include "nemisis/ui/GameMenu.hpp"
#if NEMISIS_HAS_RMLUI
#include "nemisis/ui/UiShowcase.hpp"
#endif
#include "nemisis/weapons/WeaponAttachments.hpp"
#include "nemisis/weapons/WeaponSystem.hpp"

#include "novacore/core/Application.hpp"
#include "novacore/core/ConfigRegistry.hpp"
#include "novacore/ecs/World.hpp"
#include "novacore/assets/AssetStreamer.hpp"
#include "novacore/platform/InputAction.hpp"
#include "novacore/platform/Input.hpp"
#include "novacore/platform/Window.hpp"
#include "novacore/render/Renderer.hpp"

#include <array>
#include <atomic>
#include <exception>
#include <thread>
#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

namespace nemisis::game {

inline constexpr std::uint64_t kRangeProfileWarmupFrames = 12;

struct GameAppOptions final {
    bool fullGame = false;
    bool startInFacility = false;
    bool profileRange = false;
    bool preferVulkanRenderer = true;
    bool requireVulkanRenderer = true;
    bool autoEnterDevRange = false;
    bool lockDevRange = false;
    bool runMenuFlowSmoke = false;
    bool runDeploymentSmoke = false;
    bool uiShowcase = false;
    int uiPreviewWidth = 1280;
    int uiPreviewHeight = 720;
    bool firstPersonHandIK = false;
    bool projectFPSPreview = false;
    bool projectRifleReady = false;
    bool projectFPSLookDown = false;
    bool physicsProps = false;
    bool directionalShadows = false;
    bool materialTextures = false;
};

class GameApp final : public novacore::core::IApplicationDelegate {
public:
    explicit GameApp(GameAppOptions options = {});
    ~GameApp() override;

    void onStartup() override;
    void onShutdown() override;
    void onFixedTick(const novacore::core::FrameContext& context) override;
    void onFrame(const novacore::core::FrameContext& context) override;
    [[nodiscard]] bool shouldQuit() const override;
    [[nodiscard]] bool isHeadless() const override;

private:
    [[nodiscard]] bool usesProjectRig() const;
    void enterPlayableMap();
    void loadPlayableResources(ui::GameScreen screen);
    [[nodiscard]] bool updatePlayableResourceLoading();
    void finishPlayableResources(ui::GameScreen screen);
    void stopAssetImport();
    void updateVisiblePlayerPose(float animationDelta);
    void loadAssetCatalog();
    void applyConfig(std::string_view name);
    void applyLoadedConfigs();
    void loadUserSettings();
    void persistUserSettingsIfChanged();
    void rebuildActiveAttachmentSummary();
    void syncRuntimeLoadout();
    void processGameplayWeaponInteractions(const player::PlayerInputCommand& command);
    void ensureActiveWeapon(weapons::WeaponRuntimeState& weaponState, const weapons::WeaponDefinition& effectiveWeapon);
    void ensureLocalPlayer();
    void resetDevRangeState();
    void tickRangeSession(float fixedDeltaSeconds);
    void advanceMenuFlowSmoke(const novacore::core::FrameContext& context);
    void syncRelativeMouseMode();
    void registerDevMeshResources(std::span<const std::string_view> assetIds, bool includePreview = true);
    void releaseDevMeshResources();
    [[nodiscard]] dev::DevRangePlayerRenderState currentPlayerRenderState() const;

    GameAppOptions options_;
    std::chrono::steady_clock::time_point lastProfileFrameEnd_{};
    std::uint64_t lastProfileTick_ = 0;
    novacore::platform::InputSystem input_;
    novacore::platform::InputActionMap actions_;
    input::GameplayInputBuffer gameplayInputBuffer_;
    novacore::core::ConfigRegistry configRegistry_;
    novacore::platform::Window window_;
    novacore::render::Renderer renderer_;
    novacore::ecs::World world_;
    nemisis::assets::GameAssetCatalog assetCatalog_;
    novacore::assets::AssetStreamer assetStreamer_;
    nemisis::assets::DevAssetBindings devAssetBindings_;
    nemisis::assets::DevAssetBindingSummary devAssetSummary_;
    std::unordered_map<std::string, novacore::render::MeshResourceHandle> devMeshResources_;
    novacore::ecs::EntityId cameraEntity_;
    dev::DevTargetRangeState targetRange_ = dev::makeDefaultDevTargetRange();
    dev::DevRangeSessionState devRangeSession_;
    dev::DevRangeSessionTuning devRangeTuning_;
    dev::DevSandbox devSandbox_;
    dev::GreyboxWorld greyboxWorld_ = dev::createDevRangeGreyboxWorld();
    novacore::physics::PhysicsWorld staticCollision_;
    std::unique_ptr<dev::DevRangeRigidBodies> physicsProps_;
    dev::DevRangeRenderSceneBuilder devRangeRenderer_;
    bool playableResourcesLoaded_ = false;
    bool rangeResourcesLoaded_ = false;
    dev::DevRangeRenderSceneStats latestDevRangeRenderStats_;
    ui::GameMenu menu_;
#if NEMISIS_HAS_RMLUI
    std::unique_ptr<ui::UiShowcase> uiShowcase_;
#endif
    bool uiQuitRequested_ = false;
    bool uiConfirmReleasePending_ = false;
    settings::GameSettings settings_;
    movement::MovementSystem movement_;
    render::DevRenderTuning renderTuning_;
    net::LoopbackCommandBridge loopbackBridge_;
    net::PlayerPredictionHistory predictionHistory_;
    net::PlayerSnapshotInterpolator localSnapshotInterpolator_;
    std::optional<net::PlayerSnapshotState> pendingAuthoritativeSnapshot_;
    player::PlayerCommandQueue localCommandQueue_;
    player::CharacterAnimationState characterAnimation_;
    player::CharacterAnimationFrame latestCharacterAnimationFrame_;
    float pendingAnimationSeconds_ = 0.0F;
    player::PlayerSkeletalAnimator skeletalBodyAnimator_;
    player::PlayerSkeletalAnimator skeletalArmsAnimator_;
    player::PlayerSkeletalAnimator alternateArmsAnimator_;
    std::optional<dev::FirstPersonPresentation> firstPersonPresentation_;
    std::optional<dev::FirstPersonShotFeedback> firstPersonShotFeedback_;
    std::uint64_t armVertexUpdateCount_ = 0;
    std::uint64_t fpsBodyVertexUpdateCount_ = 0;
    std::uint64_t worldBodyVertexUpdateCount_ = 0;
    bool projectFPSBodySubmitted_ = false;
    std::uint64_t projectFPSRenderFrames_ = 0;
    bool projectFPSArmsSubmitted_ = false;
    player::CameraRigState cameraRig_;
    player::AccountStats accountStats_ = player::prototypeAccountStats();
    novacore::ecs::EntityId localPlayerEntity_;
    weapons::WeaponSystem weapons_;
    weapons::AttachmentRegistry attachmentRegistry_;
    weapons::WeaponLoadout activeLoadout_ = weapons::defaultPrototypeLoadout();
    weapons::AttachmentBuildSummary activeAttachmentBuild_;
    std::filesystem::path userSettingsPath_ = settings::defaultUserSettingsPath();
    std::string lastPersistedUserSettings_;
    std::size_t menuFlowSmokeStage_ = static_cast<std::size_t>(-1);
    bool relativeMouseDesired_ = false;
    bool hasCharacterAnimationFrame_ = false;

    struct ImportResult final {
        assets::DevAssetBindingSummary summary;
        std::vector<std::string_view> imported;
        std::exception_ptr error;
        bool catalogReady = false;
    };
    ImportResult importResult_;
    std::atomic_bool importFinished_{false};
    // The barrier is enabled only by the deterministic deployment smoke.
    std::atomic_bool deploymentSmokeAllowImport_{false};
    std::atomic_bool deploymentRangeOnlyImported_{false};
    std::vector<std::string_view> pendingMeshRegistrations_;
    ui::GameScreen importTarget_ = ui::GameScreen::MainMenu;
    bool assetCatalogInitialized_ = false;
    bool previewResourcesRegistered_ = false;
    std::uint64_t deploymentLoadingFrames_ = 0;
    std::uint64_t deploymentWorkerFrames_ = 0;
    std::uint32_t deploymentPlayableFrames_ = 0;
    std::uint32_t deploymentMenuFrames_ = 0;
    std::uint32_t deploymentSmokeStage_ = 0;
    // Must be destroyed before every member the worker accesses, even if
    // Application::run exits through an exception without onShutdown().
    std::jthread assetImportThread_;
};

} // namespace nemisis::game
