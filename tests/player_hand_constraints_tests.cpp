#include "nemisis/dev/DevRangeRenderScene.hpp"
#include "nemisis/player/PlayerHandConstraints.hpp"
#include "nemisis/player/PlayerSkeletalAnimator.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace {
using namespace novacore;
using namespace nemisis::player;
int failures = 0;
void expect(bool condition, const char* message) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
float distance(math::Vec3 a, math::Vec3 b) { return std::sqrt((a - b).lengthSquared()); }
math::Vec3 unit(math::Vec3 value) { return value * (1.0F / std::sqrt(value.lengthSquared())); }
bool near(const animation::Mat4& a, const animation::Mat4& b) {
    for (std::size_t i = 0; i < a.values.size(); ++i) {
        if (!std::isfinite(a.values[i]) || !std::isfinite(b.values[i]) || std::abs(a.values[i] - b.values[i]) > 0.00001F) return false;
    }
    return true;
}
bool samePose(const animation::LocalPose& a, const animation::LocalPose& b) {
    if (a.joints.size() != b.joints.size()) return false;
    for (std::size_t i = 0; i < a.joints.size(); ++i) {
        if (!near(animation::matrixFromTransform(a.joints[i]), animation::matrixFromTransform(b.joints[i]))) return false;
    }
    return true;
}
bool sameGlobals(const animation::GlobalPose& a, const animation::GlobalPose& b) {
    if (a.joints.size() != b.joints.size()) return false;
    for (std::size_t i = 0; i < a.joints.size(); ++i) if (!near(a.joints[i], b.joints[i])) return false;
    return true;
}
float meshDifference(const assets::GltfMeshData& a, const assets::GltfMeshData& b) {
    if (a.primitives.size() != b.primitives.size()) return std::numeric_limits<float>::infinity();
    float maximum = 0;
    for (std::size_t p = 0; p < a.primitives.size(); ++p) {
        if (a.primitives[p].positions.size() != b.primitives[p].positions.size()) return std::numeric_limits<float>::infinity();
        for (std::size_t v = 0; v < a.primitives[p].positions.size(); ++v) maximum = std::max(maximum, distance(a.primitives[p].positions[v], b.primitives[p].positions[v]));
    }
    return maximum;
}
struct Fixture {
    assets::GltfMeshData mesh;
    assets::GltfAnimationAsset asset;
    PlayerHandConstraintRig rig;
    render::MeshCatalog catalog;
    render::Renderer renderer;
    nemisis::dev::MeshResourceLookup lookup;
    bool initialize() {
        for (const auto& [id, path] : std::array{
                 std::pair{"chr_a1_fp_arms_01", "assets/export/gltf/characters/chr_a1_fp_arms_01.glb"},
                 std::pair{"wpn_project_rifle_m4a1", "assets/processed/normalized/project_assets/wpn_project_rifle_m4a1.glb"}}) {
            assets::GltfMeshData imported;
            if (!assets::loadGltfMeshData(std::filesystem::path(NEMISIS_SOURCE_ROOT) / path, imported).ok()) return false;
            if (std::string_view(id) == "chr_a1_fp_arms_01") {
                std::erase_if(imported.primitives, [&](const auto& primitive) {
                    const auto node = std::ranges::find_if(imported.nodes, [&](const auto& value) {
                        return primitive.nodeIndex >= 0 && value.nodeIndex == static_cast<std::size_t>(primitive.nodeIndex);
                    });
                    return node != imported.nodes.end() && (node->name == "lod0_weapon_alignment_bar" || node->name == "lod1_blockout");
                });
                mesh = imported;
            }
            assets::AssetRecord record; record.id = id; record.kind = assets::AssetKind::Mesh; record.cookedPath = path;
            assets::GltfAssetMetadata metadata; metadata.id = id; metadata.exportPath = path;
            metadata.scaleMeters = true; metadata.runtimeUpAxis = "Y"; metadata.gameplayForwardAxis = "+Z";
            metadata.license = "original_project_asset";
            lookup.emplace(id, renderer.registerMeshResource(id, imported));
            if (!catalog.registerImportedGltfAsset(record, metadata, std::move(imported)).isValid()) return false;
        }
        return mesh.animationData && assets::buildGltfAnimationAsset(*mesh.animationData, 0U, asset).ok() &&
            buildPlayerHandConstraintRig(asset.skeleton, rig);
    }
    PlayerHandTargets targets(int index, float adsOverride = -1.0F) {
        nemisis::dev::DevRangeRenderSceneDesc desc;
        desc.meshResources = &lookup; desc.meshCatalog = &catalog; desc.useHandConstraints = true;
        auto& player = desc.player;
        player.hasCameraRig = true; player.hasAnimationFrame = true;
        player.cameraPosition = {3.0F, 1.65F, -2.0F};
        player.adsAlpha = std::array{0.0F, 1.0F, 0.65F, 0.2F}[index]; player.weapon.adsAlpha = player.adsAlpha;
        if (adsOverride >= 0.0F) player.weapon.adsAlpha = player.adsAlpha = adsOverride;
        if (index >= 2) {
            player.cameraView = {37.0F, 19.0F}; player.cameraRollDegrees = -13.0F;
            player.weapon.recoilPitchOffsetDegrees = 3.0F; player.weapon.recoilYawOffsetDegrees = -2.0F;
            player.weapon.reloading = true; player.weapon.reloadProgress = 0.4F;
            player.weaponSwayOffset = {0.03F, -0.015F, -0.02F};
            player.animation.reloadAlpha = 0.8F;
            player.animation.firstPersonWeaponOffset = {0.01F, -0.02F, 0.025F};
            player.animation.firstPersonWeaponYawAddDegrees = 1.5F;
            player.animation.firstPersonWeaponPitchAddDegrees = -2.0F;
            player.animation.firstPersonWeaponRollAddDegrees = 3.0F;
        }
        const auto presentation = nemisis::dev::evaluateFirstPersonPresentation(desc);
        expect(presentation.rightGripWorld && presentation.leftGripWorld, "actual M4 has both final cooked grip frames");
        expect(presentation.armsBoundCount == 0U, "IK placement skips whole-mesh socket averaging");
        return {firstPersonAttachmentMatrix(presentation.rig.arms),
                presentation.rightGripWorld.value_or(animation::Mat4{}), presentation.leftGripWorld.value_or(animation::Mat4{})};
    }
};
void verifyLengths(const Fixture& f, const animation::GlobalPose& base, const animation::GlobalPose& solved) {
    expect(solved.joints.size() == base.joints.size(), "constraints retain the complete joint topology");
    if (solved.joints.size() != base.joints.size()) return;
    for (const auto& arm : f.rig.arms) {
        for (const auto pair : {std::pair{arm.shoulder, arm.elbow}, std::pair{arm.elbow, arm.wrist}}) {
            expect(std::abs(distance(animation::matrixTranslation(base.joints[pair.first]), animation::matrixTranslation(base.joints[pair.second])) -
                            distance(animation::matrixTranslation(solved.joints[pair.first]), animation::matrixTranslation(solved.joints[pair.second]))) < 0.00001F,
                "IK preserves each actual upper-arm and forearm length");
        }
    }
    for (const auto& joint : solved.joints) expect(near(joint, joint), "every corrected joint matrix remains finite");
}
void verifyContacts(const Fixture& f, const animation::GlobalPose& global, const PlayerHandTargets& targets) {
    for (std::size_t side = 0; side < 2; ++side) {
        animation::Mat4 socket;
        expect(animation::evaluateSocket(f.asset.skeleton, global, f.rig.arms[side].socketIndex, socket), "final authored hand socket evaluates");
        const auto actual = animation::multiply(targets.armsWorld, socket);
        const auto& goal = side == 0 ? targets.rightGripWorld : targets.leftGripWorld;
        expect(distance(animation::matrixTranslation(actual), animation::matrixTranslation(goal)) < 0.001F,
            "actual hand socket reaches its final M4 grip within one millimetre");
        for (const auto axis : {math::Vec3{0,0,1}, math::Vec3{0,1,0}}) {
            expect(distance(unit(animation::transformDirection(actual, axis)), unit(animation::transformDirection(goal, axis))) < 0.0001F,
                "authored hand socket rotation matches full weapon-grip orientation");
        }
    }
}
void testActualWeaponConstraints(Fixture& f) {
    for (const auto& arm : f.rig.arms) {
        const auto& socket = f.asset.skeleton.sockets[arm.socketIndex];
        expect(socket.local.translation.lengthSquared() > 0.0001F && std::abs(socket.local.rotation.w) < 0.99F,
            "actual hand joint and socket differ in both authored translation and rotation");
    }
    for (int index = 0; index < 4; ++index) {
        auto base = animation::makeBindPose(f.asset.skeleton);
        if (index > 0) {
            const auto clip = std::ranges::find_if(f.asset.clips, [&](const auto& value) {
                return value.name.ends_with(index == 1 ? "_idle" : "_aim");
            });
            expect(clip != f.asset.clips.end(), "actual arms retain authored Idle and Aim clips");
            if (clip == f.asset.clips.end()) continue;
            expect(animation::sampleClip(*clip, f.asset.skeleton, clip->duration * (index == 3 ? 0.7F : 0.35F), animation::WrapMode::Clamp, base), "NovaCore samples the authored pose once");
        }
        animation::GlobalPose global, solved;
        animation::LocalPose corrected;
        expect(animation::evaluateGlobalPose(f.asset.skeleton, base, global), "base local pose evaluates");
        const auto targets = f.targets(index);
        const auto result = applyPlayerHandConstraints(f.rig, f.asset.skeleton, base, global, targets, corrected, solved);
        expect(result.applied() && result.status == HandConstraintStatus::Applied && result.reached[0] && result.reached[1], "both real M4 grips are reachable for bind/Idle/Aim and hip/ADS/recoil poses");
        verifyContacts(f, solved, targets); verifyLengths(f, global, solved);
        for (std::size_t joint = 0; joint < base.joints.size() && joint < corrected.joints.size(); ++joint) {
            expect(distance(base.joints[joint].translation, corrected.joints[joint].translation) < 0.000001F &&
                   distance(base.joints[joint].scale, corrected.joints[joint].scale) < 0.000001F, "constraints change rotations, preserving local translations and scales");
        }
        assets::GltfMeshData before, after;
        expect(assets::skinGltfMesh(f.mesh, f.asset, global, before).ok() && assets::skinGltfMesh(f.mesh, f.asset, solved, after).ok(), "NovaCore skins the final constrained globals");
        std::array<bool, 2> changed{};
        expect(before.primitives.size() == after.primitives.size(), "skinning retains the filtered mesh primitive topology");
        for (std::size_t p = 0; p < before.primitives.size() && p < after.primitives.size(); ++p) {
            const auto& primitive = f.mesh.primitives[p];
            expect(before.primitives[p].indices == after.primitives[p].indices && before.primitives[p].positions.size() == after.primitives[p].positions.size(), "IK retains all indices and vertex counts");
            for (std::size_t v = 0; v < before.primitives[p].positions.size() && v < after.primitives[p].positions.size(); ++v) {
                if (primitive.skinIndex < 0 || distance(before.primitives[p].positions[v], after.primitives[p].positions[v]) < 0.0001F) continue;
                for (std::size_t side = 0; side < 2; ++side) for (std::size_t influence = 0; influence < 4; ++influence) {
                    const auto joint = primitive.joints[v][influence]; const auto& arm = f.rig.arms[side];
                    changed[side] = changed[side] || (primitive.weights[v][influence] > 0.0F && (joint == arm.shoulder || joint == arm.elbow || joint == arm.wrist));
                }
            }
        }
        expect(changed[0] && changed[1], "both independently weighted arms deform actual visible vertices");
    }
    const auto aim = std::ranges::find_if(f.asset.clips, [](const auto& clip) { return clip.name.ends_with("_aim"); });
    if (aim == f.asset.clips.end()) return;
    auto base = animation::makeBindPose(f.asset.skeleton);
    expect(animation::sampleClip(*aim, f.asset.skeleton, aim->duration * 0.5F, animation::WrapMode::Clamp, base), "continuous ADS uses one fixed sampled Aim pose");
    animation::GlobalPose global, previous; expect(animation::evaluateGlobalPose(f.asset.skeleton, base, global), "ADS sweep base globals evaluate");
    const auto bendNormal = [](const animation::GlobalPose& pose, const HandIKArm& arm) {
        const auto upper = animation::matrixTranslation(pose.joints[arm.elbow]) - animation::matrixTranslation(pose.joints[arm.shoulder]);
        const auto lower = animation::matrixTranslation(pose.joints[arm.wrist]) - animation::matrixTranslation(pose.joints[arm.elbow]);
        return unit({upper.y * lower.z - upper.z * lower.y, upper.z * lower.x - upper.x * lower.z, upper.x * lower.y - upper.y * lower.x});
    };
    for (int step = 0; step <= 8; ++step) {
        animation::LocalPose corrected; animation::GlobalPose solved;
        const auto targets = f.targets(2, static_cast<float>(step) / 8.0F);
        const auto result = applyPlayerHandConstraints(f.rig, f.asset.skeleton, base, global, targets, corrected, solved);
        expect(result.status == HandConstraintStatus::Applied && result.reached[0] && result.reached[1], "both real grips stay reachable throughout continuous ADS");
        verifyContacts(f, solved, targets); verifyLengths(f, global, solved);
        if (step > 0 && previous.joints.size() == solved.joints.size()) for (const auto& arm : f.rig.arms) {
            const auto a = bendNormal(previous, arm); const auto b = bendNormal(solved, arm);
            expect(a.x * b.x + a.y * b.y + a.z * b.z > 0.0F &&
                   distance(animation::matrixTranslation(previous.joints[arm.elbow]), animation::matrixTranslation(solved.joints[arm.elbow])) < 0.1F,
                "each authored side retains a continuous elbow bend throughout ADS");
        }
        previous = solved;
    }
}
void testBoundsAndInvalidInputs(Fixture& f) {
    const auto base = animation::makeBindPose(f.asset.skeleton);
    animation::GlobalPose global; expect(animation::evaluateGlobalPose(f.asset.skeleton, base, global), "bind globals evaluate for failure cases");
    PlayerHandTargets targets;
    animation::GlobalPose previous;
    for (int boundary = 0; boundary < 4; ++boundary) {
        for (std::size_t side = 0; side < 2; ++side) {
            const auto& arm = f.rig.arms[side]; auto& goal = side == 0 ? targets.rightGripWorld : targets.leftGripWorld;
            expect(animation::evaluateSocket(f.asset.skeleton, global, arm.socketIndex, goal), "boundary test uses the authored socket rotation");
            const auto shoulder = animation::matrixTranslation(global.joints[arm.shoulder]);
            const auto wrist = animation::matrixTranslation(global.joints[arm.wrist]);
            const auto offset = animation::matrixTranslation(goal) - wrist;
            const float length = distance(shoulder, animation::matrixTranslation(global.joints[arm.elbow])) + distance(animation::matrixTranslation(global.joints[arm.elbow]), wrist);
            const auto desired = boundary == 0 ? shoulder + math::Vec3{100,200,300} : boundary == 1 ? shoulder :
                shoulder + unit(wrist - shoulder) * (length * 0.999F) + math::Vec3{boundary == 3 ? 0.00001F : 0.0F,0,0};
            goal.at(0,3) = desired.x + offset.x; goal.at(1,3) = desired.y + offset.y; goal.at(2,3) = desired.z + offset.z;
        }
        animation::LocalPose corrected; animation::GlobalPose solved;
        const auto result = applyPlayerHandConstraints(f.rig, f.asset.skeleton, base, global, targets, corrected, solved);
        expect(result.applied(), "unreachable and nearly straight targets produce controlled bounded corrections");
        if (boundary < 2) expect(result.status == HandConstraintStatus::Unreachable && !result.reached[0] && !result.reached[1], "far and shoulder-coincident targets report both arms unreachable");
        for (const auto residual : result.residualMeters) expect(std::isfinite(residual), "unreachable residual is finite");
        verifyLengths(f, global, solved);
        if (boundary == 3 && previous.joints.size() == solved.joints.size()) {
            for (const auto& arm : f.rig.arms) expect(distance(animation::matrixTranslation(previous.joints[arm.elbow]), animation::matrixTranslation(solved.joints[arm.elbow])) < 0.01F, "tiny near-extension target motion does not flip either elbow");
        }
        previous = solved;
    }
    for (int invalid = 0; invalid < 6; ++invalid) {
        auto rig = f.rig; auto badTargets = f.targets(0); auto skeleton = f.asset.skeleton;
        if (invalid == 0) rig.arms[1].wrist = animation::invalidJointIndex;
        if (invalid == 1) badTargets.armsWorld = animation::matrixFromTransform({{}, {}, {0,0,0}});
        if (invalid == 2) badTargets.rightGripWorld.at(0,3) = std::numeric_limits<float>::quiet_NaN();
        if (invalid == 3) rig.arms[0].socketIndex = skeleton.sockets.size();
        if (invalid == 4) skeleton.joints[rig.arms[1].wrist].parent = static_cast<int>(rig.arms[1].shoulder);
        if (invalid == 5) badTargets.armsWorld = animation::matrixFromTransform({{}, {}, {1.0F,1.0F,1.2F}});
        animation::LocalPose corrected; animation::GlobalPose solved;
        const auto result = applyPlayerHandConstraints(rig, skeleton, base, global, badTargets, corrected, solved);
        expect(result.status == HandConstraintStatus::Invalid && !result.applied(), "malformed chain, socket, transform and non-finite targets are rejected");
        expect(samePose(base, corrected) && sameGlobals(global, solved), "invalid input transactionally retains both base poses");
    }
}
void testAnimatorUsesOneFinalPose(Fixture& f) {
    PlayerSkeletalAnimator constrained, legacy, explicitLegacy;
    expect(constrained.initialize(f.mesh) && legacy.initialize(f.mesh) && explicitLegacy.initialize(f.mesh), "actual filtered arms initialize through the production animator");
    CharacterAnimationFrame frame; frame.locomotionClip = CharacterAnimationClip::Ads;
    for (int index = 0; index < 4; ++index) {
        const auto targets = f.targets(index); const auto before = constrained.stats();
        expect(constrained.update(frame, 1.0F / 60.0F, &targets) && legacy.update(frame, 1.0F / 60.0F) && explicitLegacy.update(frame, 1.0F / 60.0F, nullptr), "productive constraints and unchanged legacy paths update successfully");
        const auto& stats = constrained.stats();
        expect(stats.sampledFrameCount == before.sampledFrameCount + 1 && stats.skinnedFrameCount == before.skinnedFrameCount + 1 && stats.evaluatedFrameCount == before.evaluatedFrameCount + 1, "one update advances animation and skins exactly once");
        expect(stats.handConstraints.status == HandConstraintStatus::Applied, "production animator reports applied real-grip constraints");
        expect(legacy.stats().handConstraints.status == HandConstraintStatus::Disabled && meshDifference(*legacy.skinnedMesh(), *explicitLegacy.skinnedMesh()) < 0.000001F, "default and explicit-null paths preserve identical legacy animation");
        if (!constrained.globalPose() || !constrained.skinnedMesh()) { expect(false, "final constrained geometry is ready"); continue; }
        verifyContacts(f, *constrained.globalPose(), targets);
        for (std::size_t side = 0; side < 2; ++side) {
            animation::Mat4 actual, expected;
            const auto& socket = f.asset.skeleton.sockets[f.rig.arms[side].socketIndex];
            expect(constrained.socketTransform(socket.name, actual) && animation::evaluateSocket(f.asset.skeleton, *constrained.globalPose(), f.rig.arms[side].socketIndex, expected) && near(actual, expected), "animator socket access exposes its final corrected globals");
        }
        assets::GltfMeshData oracle;
        expect(assets::skinGltfMesh(f.mesh, f.asset, *constrained.globalPose(), oracle).ok() && meshDifference(oracle, *constrained.skinnedMesh()) < 0.000001F, "uploaded source geometry is skinned from the same final globals exposed to sockets");
        expect(meshDifference(*legacy.skinnedMesh(), *constrained.skinnedMesh()) > 0.001F, "productive IK changes weighted geometry rather than only debug sockets");
    }
}
} // namespace
int main() {
    Fixture fixture;
    const bool ready = fixture.initialize(); expect(ready, "actual M4 and filtered weighted arms import into one NovaCore rig");
    if (ready) { testActualWeaponConstraints(fixture); testBoundsAndInvalidInputs(fixture); testAnimatorUsesOneFinalPose(fixture); }
    if (failures) { std::cerr << failures << " player hand constraint checks failed\n"; return 1; }
    std::cout << "Player hand constraint tests passed\n";
    return 0;
}
