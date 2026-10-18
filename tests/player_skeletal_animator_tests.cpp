#include "nemisis/player/PlayerSkeletalAnimator.hpp"

#include "novacore/assets/GltfDocument.hpp"

#include <cmath>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

namespace {

int failures = 0;

void expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

[[nodiscard]] bool positionsDiffer(
    const novacore::assets::GltfMeshData& lhs,
    const novacore::assets::GltfMeshData& rhs) {
    if (lhs.primitives.size() != rhs.primitives.size()) {
        return true;
    }
    for (std::size_t primitiveIndex = 0; primitiveIndex < lhs.primitives.size(); ++primitiveIndex) {
        const auto& a = lhs.primitives[primitiveIndex].positions;
        const auto& b = rhs.primitives[primitiveIndex].positions;
        if (a.size() != b.size()) {
            return true;
        }
        for (std::size_t vertexIndex = 0; vertexIndex < a.size(); vertexIndex += 97U) {
            const auto delta = a[vertexIndex] - b[vertexIndex];
            if (delta.lengthSquared() > 0.0000001F) {
                return true;
            }
        }
    }
    return false;
}

void testCookedCharacterImportsAndAnimates() {
    const auto path = std::filesystem::path(NEMISIS_SOURCE_ROOT) /
        "assets/export/gltf/characters/chr_a1_stylized_operator_01.glb";
    novacore::assets::GltfMeshData mesh{};
    const auto load = novacore::assets::loadGltfMeshData(path, mesh);
    expect(load.ok(), "cooked A1 character mesh imports");
    if (!load.ok()) {
        return;
    }

    nemisis::player::PlayerSkeletalAnimator animator;
    expect(animator.initialize(mesh), "cooked character skeletal animator initializes");
    const auto initialStats = animator.stats();
    expect(initialStats.jointCount > 10U, "cooked character exposes a real joint hierarchy");
    expect(initialStats.clipCount >= 2U, "cooked character exposes authored looping animation clips");
    expect(initialStats.skinnedVertexCount > 400U, "cooked character has weighted skinned vertices");

    const auto* initialMesh = animator.skinnedMesh();
    expect(initialMesh != nullptr, "initial bind/idle pose produces a skinned mesh");
    if (initialMesh == nullptr) {
        return;
    }
    const auto idleMesh = *initialMesh;

    nemisis::player::CharacterAnimationFrame walk{};
    walk.locomotionClip = nemisis::player::CharacterAnimationClip::Walk;
    walk.stride01 = 0.65F;
    for (int frame = 0; frame < 8; ++frame) {
        expect(animator.update(walk, 1.0F / 60.0F), "walk pose evaluates and skins");
    }
    expect(
        animator.stats().currentClip.find("walk") != std::string::npos ||
            animator.stats().currentClip.find("Walk") != std::string::npos,
        "walk state selects authored Walk clip");
    expect(positionsDiffer(idleMesh, *animator.skinnedMesh()), "walk clip deforms character vertices away from idle pose");

    expect(animator.stats().evaluatedFrameCount >= 8U, "skeletal runtime tracks evaluated frames");

    auto layered = walk;
    layered.upperBodyClip = nemisis::player::CharacterAnimationClip::Ads;
    layered.upperBodyAlpha = 0.85F;
    layered.upperBodyNormalizedTime = 0.5F;
    expect(animator.update(layered, 1.0F / 60.0F), "upper-body aim layer evaluates over locomotion");
    expect(!animator.stats().upperBodyClip.empty(), "skeletal runtime reports the authored upper-body clip");
    expect(animator.stats().upperBodyWeight > 0.84F, "skeletal runtime preserves gameplay upper-body blend weight");

    novacore::animation::Mat4 hand{};
    expect(animator.socketTransform("socket_hand_r", hand), "right-hand runtime socket follows the animated skeleton");
}

void testSelectedCharacterAndArmsKeepAuthoredBindHierarchy() {
    using namespace novacore;
    for (const auto relativePath : {
        "assets/export/gltf/characters/chr_a1_fp_arms_01.glb",
        "assets/processed/normalized/project_assets/chr_project_male1.glb"}) {
        assets::GltfMeshData mesh;
        const auto load = assets::loadGltfMeshData(std::filesystem::path(NEMISIS_SOURCE_ROOT) / relativePath, mesh);
        expect(load.ok() && mesh.animationData != nullptr, "real selected character/arms has imported skin data");
        if (!load.ok() || mesh.animationData == nullptr) {
            continue;
        }
        assets::GltfAnimationAsset asset;
        const auto bridge = assets::buildGltfAnimationAsset(*mesh.animationData, 0U, asset);
        expect(bridge.ok(), "actual character/arms animation bridge succeeds");
        if (!bridge.ok()) {
            continue;
        }
        animation::GlobalPose bind;
        expect(animation::evaluateGlobalPose(asset.skeleton, animation::makeBindPose(asset.skeleton), bind), "actual complete bind hierarchy evaluates");
        for (const auto name : {"socket_camera", "socket_weapon_root", "socket_hand_r", "socket_hand_l"}) {
            const auto marker = std::ranges::find_if(mesh.nodeMarkers, [name](const auto& node) { return node.name == name; });
            animation::Mat4 socket;
            const bool found = animation::evaluateSocket(asset.skeleton, bind, name, socket);
            expect(found && marker != mesh.nodeMarkers.end(), "selected asset has each required authored attachment");
            if (found && marker != mesh.nodeMarkers.end()) {
                expect((animation::matrixTranslation(socket) - marker->worldPosition).lengthSquared() < 0.000001F, "animated bind socket position matches static scene hierarchy");
                for (const auto axis : {math::Vec3{0,0,1}, math::Vec3{0,1,0}}) {
                    const auto direction = animation::transformDirection(socket, axis);
                    const auto length = std::sqrt(direction.lengthSquared());
                    const auto expected = axis.z > 0.0F ? marker->worldForward : marker->worldUp;
                    expect(length > 0.00001F && (direction * (1.0F / length) - expected).lengthSquared() < 0.000001F, "animated bind socket orientation matches authored node frame");
                }
            }
        }
        assets::GltfMeshData skinned;
        expect(assets::skinGltfMesh(mesh, asset, bind, skinned).ok(), "actual bind-pose skinning succeeds");
        float maxBindDrift = 0.0F;
        for (std::size_t p = 0; p < mesh.primitives.size() && p < skinned.primitives.size(); ++p) {
            if (mesh.primitives[p].skinIndex != 0) {
                continue;
            }
            const auto& before = mesh.primitives[p].positions;
            const auto& after = skinned.primitives[p].positions;
            expect(before.size() == after.size(), "bind skinning retains topology");
            for (std::size_t v = 0; v < before.size() && v < after.size(); ++v) {
                maxBindDrift = std::max(maxBindDrift, (before[v] - after[v]).lengthSquared());
            }
        }
        expect(maxBindDrift < 0.000001F, "full joint globals cancel inverse binds rather than rotating geometry away from sockets");

        nemisis::player::PlayerSkeletalAnimator animator;
        expect(animator.initialize(mesh), "selected character/arms initializes one skeletal runtime");
        if (!animator.stats().initialized || animator.skinnedMesh() == nullptr) {
            continue;
        }
        const auto initial = *animator.skinnedMesh();
        nemisis::player::CharacterAnimationFrame frame;
        const bool selectedCharacter = std::string_view(relativePath).find("chr_project_male1") != std::string_view::npos;
        frame.locomotionClip = selectedCharacter
            ? nemisis::player::CharacterAnimationClip::Walk : nemisis::player::CharacterAnimationClip::Ads;
        frame.stride01 = 0.65F;
        const auto started = std::chrono::steady_clock::now();
        for (int i = 0; i < 12; ++i) {
            expect(animator.update(frame, 1.0F / 60.0F), "selected character/arms updates pose and skinned geometry");
        }
        const auto frameMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count() / 12.0;
        expect(animator.stats().evaluatedFrameCount == 12U, "exactly one completed pose/skin update per requested frame");
        expect(positionsDiffer(initial, *animator.skinnedMesh()), "authored movement/aim visibly changes actual mesh vertex positions");
        std::cout << "skeletal CPU sample: " << relativePath << " joints=" << animator.stats().jointCount
            << " vertices=" << animator.stats().skinnedVertexCount << " frame_ms=" << frameMs
            << " max_bind_drift=" << std::sqrt(maxBindDrift) << '\n';
    }
}

} // namespace

int main() {
    testCookedCharacterImportsAndAnimates();
    testSelectedCharacterAndArmsKeepAuthoredBindHierarchy();
    if (failures != 0) {
        std::cerr << failures << " player skeletal animator test(s) failed\n";
        return 1;
    }
    std::cout << "player skeletal animator tests passed\n";
    return 0;
}
