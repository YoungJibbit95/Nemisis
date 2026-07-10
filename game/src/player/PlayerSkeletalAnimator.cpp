#include "nemisis/player/PlayerSkeletalAnimator.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <initializer_list>
#include <string>

namespace nemisis::player {

namespace {

[[nodiscard]] std::string lowercase(std::string_view value) {
    std::string result(value);
    std::ranges::transform(result, result.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return result;
}

[[nodiscard]] bool containsHint(std::string_view name, std::string_view hint) {
    return lowercase(name).find(lowercase(hint)) != std::string::npos;
}

[[nodiscard]] float transitionDuration(CharacterAnimationClip clip) {
    switch (clip) {
    case CharacterAnimationClip::Jump:
    case CharacterAnimationClip::Fall:
    case CharacterAnimationClip::Land:
    case CharacterAnimationClip::Slide:
    case CharacterAnimationClip::WallRun:
    case CharacterAnimationClip::Mantle:
        return 0.08F;
    case CharacterAnimationClip::Sprint:
        return 0.12F;
    case CharacterAnimationClip::Idle:
    case CharacterAnimationClip::Walk:
    case CharacterAnimationClip::Airborne:
    case CharacterAnimationClip::Crouch:
        return 0.16F;
    case CharacterAnimationClip::Reload:
    case CharacterAnimationClip::Ads:
    case CharacterAnimationClip::Fire:
        return 0.10F;
    }
    return 0.14F;
}

} // namespace

bool PlayerSkeletalAnimator::initialize(const novacore::assets::GltfMeshData& meshData) {
    reset();
    sourceMesh_ = meshData;

    novacore::assets::GltfAnimationData imported{};
    if (meshData.animationData != nullptr) {
        imported = *meshData.animationData;
    } else {
        const auto load = novacore::assets::loadGltfAnimationData(meshData.path, imported);
        if (!load.ok()) {
            errors_ = load.errors;
            return false;
        }
    }
    if (imported.skins.empty()) {
        errors_.push_back("Character mesh has no glTF skin");
        return false;
    }

    const auto bridge = novacore::assets::buildGltfAnimationAsset(imported, 0U, animationAsset_);
    warnings_ = bridge.warnings;
    errors_ = bridge.errors;
    if (!bridge.ok()) {
        return false;
    }

    const auto skeletonValidation = runtime_.setSkeleton(animationAsset_.skeleton);
    if (!skeletonValidation.valid()) {
        for (const auto& issue : skeletonValidation.issues) {
            if (issue.severity == novacore::animation::ValidationSeverity::Error) {
                errors_.push_back(issue.path + ": " + issue.message);
            }
        }
        return false;
    }

    activeClip_ = selectClip(CharacterAnimationClip::Idle);
    if (activeClip_ == nullptr && !animationAsset_.clips.empty()) {
        activeClip_ = &animationAsset_.clips.front();
    }
    if (activeClip_ == nullptr ||
        !runtime_.play(*activeClip_, novacore::animation::WrapMode::Loop).valid()) {
        errors_.push_back("Character skin has no playable animation clip");
        return false;
    }

    if (!runtime_.update(0.0F)) {
        errors_.push_back("Initial character animation pose evaluation failed");
        return false;
    }

    const auto upperRoot = std::ranges::find_if(
        animationAsset_.skeleton.joints,
        [](const novacore::animation::Joint& joint) {
            const auto name = lowercase(joint.name);
            return name.find("chest") != std::string::npos ||
                name.find("spine_02") != std::string::npos ||
                name.find("spine2") != std::string::npos ||
                name == "spine";
        });
    if (upperRoot != animationAsset_.skeleton.joints.end()) {
        const auto jointIndex = static_cast<novacore::animation::JointIndex>(
            std::distance(animationAsset_.skeleton.joints.begin(), upperRoot));
        upperBodyMask_ = novacore::animation::makeDescendantMask(animationAsset_.skeleton, jointIndex);
        upperBodyMaskReady_ = novacore::animation::validateLayerMask(
            upperBodyMask_, animationAsset_.skeleton).valid();
    }
    const auto skin = novacore::assets::skinGltfMesh(
        sourceMesh_, animationAsset_, runtime_.globalPose(), skinnedMesh_);
    warnings_.insert(warnings_.end(), skin.warnings.begin(), skin.warnings.end());
    errors_.insert(errors_.end(), skin.errors.begin(), skin.errors.end());
    if (!skin.ok()) {
        return false;
    }

    stats_.jointCount = animationAsset_.skeleton.joints.size();
    stats_.socketCount = animationAsset_.skeleton.sockets.size();
    stats_.clipCount = animationAsset_.clips.size();
    stats_.skinnedPrimitiveCount = std::ranges::count_if(
        skinnedMesh_.primitives,
        [](const novacore::assets::GltfPrimitiveData& primitive) {
            return primitive.skinIndex >= 0;
        });
    stats_.skinnedVertexCount = 0;
    for (const auto& primitive : skinnedMesh_.primitives) {
        if (primitive.skinIndex >= 0) {
            stats_.skinnedVertexCount += primitive.positions.size();
        }
    }
    stats_.currentClip = activeClip_->name;
    stats_.initialized = true;
    stats_.poseReady = true;
    return true;
}

void PlayerSkeletalAnimator::reset() {
    sourceMesh_ = {};
    skinnedMesh_ = {};
    animationAsset_ = {};
    runtime_ = {};
    activeClip_ = nullptr;
    upperBodyMask_ = {};
    upperBodyMaskReady_ = false;
    stats_ = {};
    warnings_.clear();
    errors_.clear();
}

bool PlayerSkeletalAnimator::update(
    const CharacterAnimationFrame& frame,
    float deltaSeconds) {
    if (!stats_.initialized) {
        return false;
    }

    const auto* requestedClip = selectClip(frame.locomotionClip);
    if (requestedClip != nullptr && requestedClip != activeClip_ &&
        !activateClip(*requestedClip, transitionDuration(frame.locomotionClip))) {
        ++stats_.failedFrameCount;
        stats_.poseReady = false;
        return false;
    }

    const float playbackScale = frame.locomotionClip == CharacterAnimationClip::Sprint
        ? std::clamp(0.85F + frame.stride01 * 0.55F, 0.85F, 1.40F)
        : std::clamp(0.75F + frame.stride01 * 0.45F, 0.75F, 1.20F);

    const auto* upperClip = selectClip(frame.upperBodyClip);
    const float upperWeight = std::clamp(frame.upperBodyAlpha, 0.0F, 1.0F);
    if (upperBodyMaskReady_ && upperClip != nullptr && upperWeight > 0.001F && upperClip != activeClip_) {
        const auto layerValidation = runtime_.setLayer(
            0U,
            novacore::animation::AnimationLayerDesc{
                upperClip,
                upperBodyMask_,
                std::clamp(frame.upperBodyNormalizedTime, 0.0F, 1.0F) * upperClip->duration,
                1.0F,
                upperWeight,
                frame.upperBodyClip == CharacterAnimationClip::Reload ||
                        frame.upperBodyClip == CharacterAnimationClip::Fire
                    ? novacore::animation::WrapMode::Clamp
                    : novacore::animation::WrapMode::Loop,
            });
        if (!layerValidation.valid()) {
            errors_.push_back("Upper-body animation layer validation failed");
            ++stats_.failedFrameCount;
            stats_.poseReady = false;
            return false;
        }
        stats_.upperBodyClip = upperClip->name;
        stats_.upperBodyWeight = upperWeight;
    } else {
        runtime_.clearLayer(0U);
        stats_.upperBodyClip.clear();
        stats_.upperBodyWeight = 0.0F;
    }
    if (!runtime_.update(std::clamp(deltaSeconds * playbackScale, 0.0F, 0.10F))) {
        errors_.push_back("Animation runtime update failed: " + std::string(runtime_.lastError()));
        ++stats_.failedFrameCount;
        stats_.poseReady = false;
        return false;
    }

    const auto skin = novacore::assets::skinGltfMesh(
        sourceMesh_, animationAsset_, runtime_.globalPose(), skinnedMesh_);
    if (!skin.ok()) {
        errors_.insert(errors_.end(), skin.errors.begin(), skin.errors.end());
        ++stats_.failedFrameCount;
        stats_.poseReady = false;
        return false;
    }

    ++stats_.evaluatedFrameCount;
    stats_.poseReady = true;
    stats_.currentClip = activeClip_ != nullptr ? activeClip_->name : std::string{};
    return true;
}

const novacore::assets::GltfMeshData* PlayerSkeletalAnimator::skinnedMesh() const {
    return stats_.poseReady ? &skinnedMesh_ : nullptr;
}

const novacore::animation::GlobalPose* PlayerSkeletalAnimator::globalPose() const {
    return stats_.poseReady ? &runtime_.globalPose() : nullptr;
}

bool PlayerSkeletalAnimator::socketTransform(
    std::string_view socketName,
    novacore::animation::Mat4& outTransform) const {
    return stats_.poseReady && runtime_.socketTransform(socketName, outTransform);
}

const PlayerSkeletalAnimatorStats& PlayerSkeletalAnimator::stats() const {
    return stats_;
}

const std::vector<std::string>& PlayerSkeletalAnimator::warnings() const {
    return warnings_;
}

const std::vector<std::string>& PlayerSkeletalAnimator::errors() const {
    return errors_;
}

const novacore::animation::AnimationClip* PlayerSkeletalAnimator::selectClip(
    CharacterAnimationClip clip) const {
    switch (clip) {
    case CharacterAnimationClip::Idle:
    case CharacterAnimationClip::Land:
    case CharacterAnimationClip::Crouch:
        return findClipByHints({"idle", "hard stand", "stand", "aim"});
    case CharacterAnimationClip::Walk:
        return findClipByHints({"walk"});
    case CharacterAnimationClip::Sprint:
    case CharacterAnimationClip::Slide:
    case CharacterAnimationClip::Airborne:
    case CharacterAnimationClip::WallRun:
    case CharacterAnimationClip::Mantle:
    case CharacterAnimationClip::Jump:
    case CharacterAnimationClip::Fall:
        if (const auto* run = findClipByHints({"run", "sprint"}); run != nullptr) {
            return run;
        }
        return findClipByHints({"walk", "idle", "hard stand"});
    case CharacterAnimationClip::Reload:
        if (const auto* reload = findClipByHints({"reload"}); reload != nullptr) {
            return reload;
        }
        return findClipByHints({"aim", "idle", "hard stand"});
    case CharacterAnimationClip::Ads:
    case CharacterAnimationClip::Fire:
        return findClipByHints({"aim", "idle", "hard stand"});
    }
    return nullptr;
}

const novacore::animation::AnimationClip* PlayerSkeletalAnimator::findClipByHints(
    std::initializer_list<std::string_view> hints) const {
    for (const auto hint : hints) {
        const auto found = std::ranges::find_if(
            animationAsset_.clips,
            [hint](const novacore::animation::AnimationClip& clip) {
                return containsHint(clip.name, hint);
            });
        if (found != animationAsset_.clips.end()) {
            return &*found;
        }
    }
    return nullptr;
}

bool PlayerSkeletalAnimator::activateClip(
    const novacore::animation::AnimationClip& clip,
    float transitionSeconds) {
    const auto validation = activeClip_ == nullptr
        ? runtime_.play(clip, novacore::animation::WrapMode::Loop)
        : runtime_.crossFade(
            clip,
            std::max(0.0F, transitionSeconds),
            novacore::animation::WrapMode::Loop);
    if (!validation.valid()) {
        for (const auto& issue : validation.issues) {
            if (issue.severity == novacore::animation::ValidationSeverity::Error) {
                errors_.push_back(issue.path + ": " + issue.message);
            }
        }
        return false;
    }
    activeClip_ = &clip;
    return true;
}

} // namespace nemisis::player
