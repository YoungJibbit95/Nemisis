#include "nemisis/player/PlayerSkeletalAnimator.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <initializer_list>
#include <string>
#include <tuple>

namespace nemisis::player {

namespace {

[[nodiscard]] std::string lowercase(std::string_view value) {
    std::string result(value);
    std::ranges::transform(result, result.begin(),
                           [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
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

bool mergeCompanion(novacore::assets::GltfAnimationData& shared, const novacore::assets::GltfAnimationData& part) {
    using namespace novacore::assets;
    // Candidate subsets retain original source node indices. Reject unrelated
    // assets rather than mapping a different hierarchy by coincidental names.
    const auto vec = [](auto v) { return std::tuple{v.x, v.y, v.z}; };
    const auto quat = [](auto q) { return std::tuple{q.x, q.y, q.z, q.w}; };
    for (const auto& node : part.nodes) {
        const auto found = std::ranges::find(shared.nodes, node.nodeIndex, &GltfNodeData::nodeIndex);
        if (found == shared.nodes.end()) {
            return false;
        }
        const auto& a = found->localTransform;
        const auto& b = node.localTransform;
        if (found->name != node.name || found->parentNodeIndex != node.parentNodeIndex ||
            a.usesMatrix != b.usesMatrix || vec(a.translation) != vec(b.translation) ||
            quat(a.rotation) != quat(b.rotation) || vec(a.scale) != vec(b.scale) ||
            a.matrix.values != b.matrix.values) {
            return false;
        }
    }
    for (const auto& joint : part.skins[0].joints) {
        auto& joints = shared.skins[0].joints;
        const auto found = std::ranges::find(joints, joint.nodeIndex, &GltfJointData::nodeIndex);
        if (found == joints.end()) {
            joints.push_back(joint);
        } else if (found->inverseBindMatrix.values != joint.inverseBindMatrix.values) {
            return false;
        }
    }
    if (part.animations.size() != shared.animations.size()) {
        return false;
    }
    for (const auto& clip : part.animations) {
        const auto found = std::ranges::find(shared.animations, clip.name, &GltfAnimationClipData::name);
        if (found == shared.animations.end() || found->durationSeconds != clip.durationSeconds) {
            return false;
        }
        for (const auto& channel : clip.channels) {
            const auto old = std::ranges::find_if(found->channels, [&](const auto& c) {
                return c.targetNodeIndex == channel.targetNodeIndex && c.targetPath == channel.targetPath;
            });
            if (old == found->channels.end()) {
                found->channels.push_back(channel);
                continue;
            }
            if (old->interpolation != channel.interpolation ||
                !std::ranges::equal(old->vectorKeyframes, channel.vectorKeyframes,
                                    [&](const auto& a, const auto& b) {
                                        return a.timeSeconds == b.timeSeconds && vec(a.value) == vec(b.value) &&
                                               vec(a.inTangent) == vec(b.inTangent) &&
                                               vec(a.outTangent) == vec(b.outTangent);
                                    }) ||
                !std::ranges::equal(
                    old->rotationKeyframes, channel.rotationKeyframes, [&](const auto& a, const auto& b) {
                        return a.timeSeconds == b.timeSeconds && quat(a.value) == quat(b.value) &&
                               quat(a.inTangent) == quat(b.inTangent) && quat(a.outTangent) == quat(b.outTangent);
                    })) {
                return false;
            }
        }
    }
    return true;
}

} // namespace

bool PlayerSkeletalAnimator::initialize(const novacore::assets::GltfMeshData& meshData,
                                        const novacore::assets::GltfMeshData* companion) {
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

    if (companion != nullptr) {
        if (!companion->animationData || companion->animationData->skins.size() != 1 || imported.skins.size() != 1 ||
            !novacore::assets::buildGltfAnimationAsset(imported, 0U, primarySkinAsset_).ok() ||
            !novacore::assets::buildGltfAnimationAsset(*companion->animationData, 0U, companionSkinAsset_).ok() ||
            !mergeCompanion(imported, *companion->animationData)) {
            errors_.push_back("Companion must share source hierarchy, inverse binds and clip keys");
            return false;
        }
        primarySkinAsset_.clips.clear();
        companionSkinAsset_.clips.clear();
        companionSource_ = *companion;
        hasCompanion_ = true;
    }
    const auto bridge = novacore::assets::buildGltfAnimationAsset(imported, 0U, animationAsset_);
    warnings_ = bridge.warnings;
    errors_ = bridge.errors;
    if (!bridge.ok()) {
        return false;
    }
    if (hasCompanion_) {
        const auto map = [&](const auto& part, auto& result) {
            for (const auto node : part.jointNodeIndices) {
                const auto found = std::ranges::find(animationAsset_.jointNodeIndices, node);
                if (found == animationAsset_.jointNodeIndices.end()) {
                    return false;
                }
                result.push_back(static_cast<std::size_t>(found - animationAsset_.jointNodeIndices.begin()));
            }
            return true;
        };
        if (!map(primarySkinAsset_, primaryPoseMap_) || !map(companionSkinAsset_, companionPoseMap_)) {
            errors_.push_back("Companion palette cannot map to the common pose");
            return false;
        }
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

    readyClip_ = findClipByHints({"developer rifle ready idle"});
    armedIdleClip_ = readyClip_ != nullptr ? findClipByHints({"project m4 armed idle"}) : nullptr;
    activeClip_ = selectClip(CharacterAnimationClip::Idle);
    if (activeClip_ == nullptr && !animationAsset_.clips.empty()) {
        activeClip_ = &animationAsset_.clips.front();
    }
    if (activeClip_ == nullptr || !runtime_.play(*activeClip_, novacore::animation::WrapMode::Loop).valid()) {
        errors_.push_back("Character skin has no playable animation clip");
        return false;
    }

    if (!runtime_.update(0.0F)) {
        errors_.push_back("Initial character animation pose evaluation failed");
        return false;
    }

    const auto upperRoot =
        std::ranges::find_if(animationAsset_.skeleton.joints, [](const novacore::animation::Joint& joint) {
            const auto name = lowercase(joint.name);
            return name.find("chest") != std::string::npos || name.find("spine_02") != std::string::npos ||
                   name.find("spine2") != std::string::npos || name == "spine";
        });
    if (upperRoot != animationAsset_.skeleton.joints.end()) {
        const auto jointIndex = static_cast<novacore::animation::JointIndex>(
            std::distance(animationAsset_.skeleton.joints.begin(), upperRoot));
        upperBodyMask_ = novacore::animation::makeDescendantMask(animationAsset_.skeleton, jointIndex);
        upperBodyMaskReady_ = novacore::animation::validateLayerMask(upperBodyMask_, animationAsset_.skeleton).valid();
    }
    if (!skinMeshes(runtime_.globalPose())) {
        return false;
    }

#if defined(NEMISIS_ENABLE_HAND_IK)
    if (!buildPlayerHandConstraintRig(animationAsset_.skeleton, handRig_) && readyClip_ != nullptr) {
        errors_.push_back("Developer Rifle Ready requires the actual Project palm sockets and deform chains");
        return false;
    }
#endif
    stats_.jointCount = animationAsset_.skeleton.joints.size();
    stats_.socketCount = animationAsset_.skeleton.sockets.size();
    stats_.clipCount = animationAsset_.clips.size();
    stats_.skinnedPrimitiveCount =
        std::ranges::count_if(skinnedMesh_.primitives, [](const novacore::assets::GltfPrimitiveData& primitive) {
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
    primarySkinAsset_ = {};
    companionSkinAsset_ = {};
    companionSource_ = {};
    companionSkinned_ = {};
    primaryPoseMap_.clear();
    companionPoseMap_.clear();
    partPose_ = {};
    hasCompanion_ = false;
    samplePending_ = false;
    runtime_ = {};
    handRig_ = {};
    constrainedLocal_ = {};
    constrainedGlobal_ = {};
    constrainedPoseReady_ = false;
    activeClip_ = nullptr;
    readyClip_ = nullptr;
    armedIdleClip_ = nullptr;
    upperBodyMask_ = {};
    upperBodyMaskReady_ = false;
    stats_ = {};
    warnings_.clear();
    errors_.clear();
}

bool PlayerSkeletalAnimator::update(const CharacterAnimationFrame& frame, float deltaSeconds,
                                    const PlayerHandTargets* handTargets) {
    return samplePose(frame, deltaSeconds) && skinPose(handTargets);
}

bool PlayerSkeletalAnimator::samplePose(const CharacterAnimationFrame& frame, float deltaSeconds) {
    if (!stats_.initialized || samplePending_) {
        return false;
    }

    const auto* requestedClip = selectClip(frame.locomotionClip);
    // NovaCore deliberately rejects overlapping fades. Finish the short active
    // fade, then consume the latest gameplay request on a subsequent tick.
    if (requestedClip != nullptr && requestedClip != activeClip_ && !runtime_.transitioning() &&
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
    if (readyClip_ == nullptr && upperBodyMaskReady_ && upperClip != nullptr && upperWeight > 0.001F &&
        upperClip != activeClip_) {
        const auto layerValidation =
            runtime_.setLayer(0U, novacore::animation::AnimationLayerDesc{
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

    ++stats_.sampledFrameCount;
    constrainedPoseReady_ = false;
    if (readyClip_ != nullptr && activeClip_ != readyClip_ && activeClip_ != armedIdleClip_) {
        constrainedLocal_ = runtime_.localPose();
        // Offline-authored rotation mask, including the actual arm helper
        // ancestors. Keep every base translation and animated scale intact.
        for (const auto& track : readyClip_->tracks) {
            if (!track.rotation.keys.empty()) {
                constrainedLocal_.joints[track.joint].rotation = track.rotation.keys.front().value;
            }
        }
        if (!novacore::animation::evaluateGlobalPose(animationAsset_.skeleton, constrainedLocal_, constrainedGlobal_)) {
            ++stats_.failedFrameCount;
            stats_.poseReady = false;
            return false;
        }
        constrainedPoseReady_ = true;
        stats_.upperBodyClip = readyClip_->name;
        stats_.upperBodyWeight = 1;
    }
    samplePending_ = true;
    stats_.poseReady = true;
    stats_.currentClip = activeClip_ != nullptr ? activeClip_->name : std::string{};
    return true;
}

bool PlayerSkeletalAnimator::skinPose(const PlayerHandTargets* handTargets) {
    if (!stats_.initialized || !samplePending_) {
        return false;
    }
    samplePending_ = false;
    stats_.handConstraints = {};
    if (handTargets != nullptr) {
#if defined(NEMISIS_ENABLE_HAND_IK)
        stats_.handConstraints = applyPlayerHandConstraints(
            handRig_, animationAsset_.skeleton, constrainedPoseReady_ ? constrainedLocal_ : runtime_.localPose(),
            constrainedPoseReady_ ? constrainedGlobal_ : runtime_.globalPose(), *handTargets, constrainedLocal_,
            constrainedGlobal_);
        constrainedPoseReady_ = constrainedPoseReady_ || stats_.handConstraints.applied();
#else
        stats_.handConstraints.status = HandConstraintStatus::Invalid;
#endif
    }
    if (stats_.handConstraints.applied()) {
        ++stats_.constrainedFrameCount;
    }
    if (stats_.handConstraints.status == HandConstraintStatus::Unreachable) {
        ++stats_.unreachableHandFrameCount;
    } else if (stats_.handConstraints.status == HandConstraintStatus::Invalid) {
        ++stats_.invalidHandFrameCount;
        if (readyClip_ != nullptr) {
            ++stats_.failedFrameCount;
            stats_.poseReady = false;
            errors_.push_back("Developer Rifle Ready hand constraints rejected the sampled pose");
            return false;
        }
    }
    const auto& finalPose = constrainedPoseReady_ ? constrainedGlobal_ : runtime_.globalPose();
    if (!skinMeshes(finalPose)) {
        ++stats_.failedFrameCount;
        stats_.poseReady = false;
        return false;
    }

    ++stats_.skinnedFrameCount;
    ++stats_.evaluatedFrameCount;
    stats_.poseReady = true;
    stats_.currentClip = activeClip_ != nullptr ? activeClip_->name : std::string{};
    return true;
}

bool PlayerSkeletalAnimator::skinMeshes(const novacore::animation::GlobalPose& pose) {
    const auto skin = [&](const auto& source, const auto& asset, const auto& mapping, auto& output) {
        partPose_.joints.resize(mapping.size());
        for (std::size_t i = 0; i < mapping.size(); ++i) {
            partPose_.joints[i] = pose.joints[mapping[i]];
        }
        const auto result = novacore::assets::skinGltfMesh(source, asset, hasCompanion_ ? partPose_ : pose, output);
        if (!result.ok()) {
            errors_.insert(errors_.end(), result.errors.begin(), result.errors.end());
        }
        return result.ok();
    };
    return skin(sourceMesh_, hasCompanion_ ? primarySkinAsset_ : animationAsset_, primaryPoseMap_, skinnedMesh_) &&
           (!hasCompanion_ || skin(companionSource_, companionSkinAsset_, companionPoseMap_, companionSkinned_));
}

const novacore::assets::GltfMeshData* PlayerSkeletalAnimator::skinnedMesh() const {
    return stats_.poseReady && !samplePending_ ? &skinnedMesh_ : nullptr;
}

const novacore::assets::GltfMeshData* PlayerSkeletalAnimator::skinnedCompanion() const {
    return hasCompanion_ && stats_.poseReady && !samplePending_ ? &companionSkinned_ : nullptr;
}

const novacore::animation::GlobalPose* PlayerSkeletalAnimator::globalPose() const {
    return stats_.poseReady ? (constrainedPoseReady_ ? &constrainedGlobal_ : &runtime_.globalPose()) : nullptr;
}

bool PlayerSkeletalAnimator::socketTransform(std::string_view socketName,
                                             novacore::animation::Mat4& outTransform) const {
    const auto* pose = globalPose();
    return pose != nullptr &&
           novacore::animation::evaluateSocket(animationAsset_.skeleton, *pose, socketName, outTransform);
}

const PlayerSkeletalAnimatorStats& PlayerSkeletalAnimator::stats() const { return stats_; }

const std::vector<std::string>& PlayerSkeletalAnimator::warnings() const { return warnings_; }

const std::vector<std::string>& PlayerSkeletalAnimator::errors() const { return errors_; }

const novacore::animation::AnimationClip* PlayerSkeletalAnimator::selectClip(CharacterAnimationClip clip) const {
    switch (clip) {
    case CharacterAnimationClip::Idle:
        if (armedIdleClip_ != nullptr) { return armedIdleClip_; }
        return findClipByHints({"idle", "hard stand", "stand", "aim"});
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

const novacore::animation::AnimationClip*
PlayerSkeletalAnimator::findClipByHints(std::initializer_list<std::string_view> hints) const {
    for (const auto hint : hints) {
        const auto found =
            std::ranges::find_if(animationAsset_.clips, [hint](const novacore::animation::AnimationClip& clip) {
                return containsHint(clip.name, hint);
            });
        if (found != animationAsset_.clips.end()) {
            return &*found;
        }
    }
    return nullptr;
}

bool PlayerSkeletalAnimator::activateClip(const novacore::animation::AnimationClip& clip, float transitionSeconds) {
    const auto validation = activeClip_ == nullptr ? runtime_.play(clip, novacore::animation::WrapMode::Loop)
                                                   : runtime_.crossFade(clip, std::max(0.0F, transitionSeconds),
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
