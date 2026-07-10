#pragma once

#include "nemisis/player/PlayerAnimation.hpp"

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
    std::string currentClip;
    bool initialized = false;
    bool poseReady = false;
};

class PlayerSkeletalAnimator final {
public:
    [[nodiscard]] bool initialize(const novacore::assets::GltfMeshData& meshData);
    void reset();

    [[nodiscard]] bool update(
        const CharacterAnimationFrame& frame,
        float deltaSeconds);

    [[nodiscard]] const novacore::assets::GltfMeshData* skinnedMesh() const;
    [[nodiscard]] const novacore::animation::GlobalPose* globalPose() const;
    [[nodiscard]] bool socketTransform(
        std::string_view socketName,
        novacore::animation::Mat4& outTransform) const;

    [[nodiscard]] const PlayerSkeletalAnimatorStats& stats() const;
    [[nodiscard]] const std::vector<std::string>& warnings() const;
    [[nodiscard]] const std::vector<std::string>& errors() const;

private:
    [[nodiscard]] const novacore::animation::AnimationClip* selectClip(
        CharacterAnimationClip clip) const;
    [[nodiscard]] const novacore::animation::AnimationClip* findClipByHints(
        std::initializer_list<std::string_view> hints) const;
    [[nodiscard]] bool activateClip(
        const novacore::animation::AnimationClip& clip,
        float transitionSeconds);

    novacore::assets::GltfMeshData sourceMesh_;
    novacore::assets::GltfMeshData skinnedMesh_;
    novacore::assets::GltfAnimationAsset animationAsset_;
    novacore::animation::AnimationRuntime runtime_;
    const novacore::animation::AnimationClip* activeClip_ = nullptr;
    PlayerSkeletalAnimatorStats stats_;
    std::vector<std::string> warnings_;
    std::vector<std::string> errors_;
};

} // namespace nemisis::player
