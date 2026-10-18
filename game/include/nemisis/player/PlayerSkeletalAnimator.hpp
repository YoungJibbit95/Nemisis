#pragma once

#include "nemisis/player/PlayerAnimation.hpp"
#include "nemisis/player/PlayerHandConstraints.hpp"

#include "novacore/animation/AnimationRuntime.hpp"
#include "novacore/assets/GltfAnimationBridge.hpp"
#include "novacore/assets/GltfDocument.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace nemisis::player {

struct PlayerSkeletalAnimatorStats final {
    std::size_t jointCount = 0;
    std::size_t socketCount = 0;
    std::size_t clipCount = 0;
    std::size_t skinnedPrimitiveCount = 0;
    std::size_t skinnedVertexCount = 0;
    std::uint64_t evaluatedFrameCount = 0;
    std::uint64_t failedFrameCount = 0;
    std::uint64_t sampledFrameCount = 0;
    std::uint64_t skinnedFrameCount = 0;
    std::uint64_t constrainedFrameCount = 0;
    std::uint64_t unreachableHandFrameCount = 0;
    std::uint64_t invalidHandFrameCount = 0;
    PlayerHandConstraintResult handConstraints{};
    std::string currentClip;
    std::string upperBodyClip;
    float upperBodyWeight = 0.0F;
    bool initialized = false;
    bool poseReady = false;
};

class PlayerSkeletalAnimator final {
  public:
    [[nodiscard]] bool initialize(const novacore::assets::GltfMeshData& meshData,
                                  const novacore::assets::GltfMeshData* companion = nullptr);
    void reset();

    [[nodiscard]] bool update(const CharacterAnimationFrame& frame, float deltaSeconds,
                              const PlayerHandTargets* handTargets = nullptr);

    // Fixed tick order: sample once, read eye, evaluate shared weapon, constrain/skin once.
    [[nodiscard]] bool samplePose(const CharacterAnimationFrame& frame, float deltaSeconds);
    [[nodiscard]] bool skinPose(const PlayerHandTargets* handTargets = nullptr);

    [[nodiscard]] const novacore::assets::GltfMeshData* skinnedMesh() const;
    [[nodiscard]] const novacore::assets::GltfMeshData* skinnedCompanion() const;
    [[nodiscard]] const novacore::animation::GlobalPose* globalPose() const;
    [[nodiscard]] bool socketTransform(std::string_view socketName, novacore::animation::Mat4& outTransform) const;

    [[nodiscard]] const PlayerSkeletalAnimatorStats& stats() const;
    [[nodiscard]] const std::vector<std::string>& warnings() const;
    [[nodiscard]] const std::vector<std::string>& errors() const;

  private:
    [[nodiscard]] const novacore::animation::AnimationClip* selectClip(CharacterAnimationClip clip) const;
    [[nodiscard]] const novacore::animation::AnimationClip*
    findClipByHints(std::initializer_list<std::string_view> hints) const;
    [[nodiscard]] bool activateClip(const novacore::animation::AnimationClip& clip, float transitionSeconds);
    [[nodiscard]] bool skinMeshes(const novacore::animation::GlobalPose& pose);

    novacore::assets::GltfMeshData sourceMesh_;
    novacore::assets::GltfMeshData skinnedMesh_;
    novacore::assets::GltfAnimationAsset animationAsset_;
    novacore::assets::GltfAnimationAsset primarySkinAsset_;
    novacore::assets::GltfAnimationAsset companionSkinAsset_;
    novacore::assets::GltfMeshData companionSource_;
    novacore::assets::GltfMeshData companionSkinned_;
    std::vector<std::size_t> primaryPoseMap_;
    std::vector<std::size_t> companionPoseMap_;
    novacore::animation::GlobalPose partPose_;
    bool hasCompanion_ = false;
    bool samplePending_ = false;
    novacore::animation::AnimationRuntime runtime_;
    PlayerHandConstraintRig handRig_{};
    novacore::animation::LocalPose constrainedLocal_;
    novacore::animation::GlobalPose constrainedGlobal_;
    bool constrainedPoseReady_ = false;
    const novacore::animation::AnimationClip* activeClip_ = nullptr;
    const novacore::animation::AnimationClip* readyClip_ = nullptr;
    const novacore::animation::AnimationClip* armedIdleClip_ = nullptr;
    novacore::animation::LayerMask upperBodyMask_;
    bool upperBodyMaskReady_ = false;
    PlayerSkeletalAnimatorStats stats_;
    std::vector<std::string> warnings_;
    std::vector<std::string> errors_;
};

} // namespace nemisis::player
