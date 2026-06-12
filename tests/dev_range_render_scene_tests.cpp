#include "nemisis/dev/DevRangeRenderScene.hpp"
#include "nemisis/dev/GreyboxWorld.hpp"

#include "novacore/assets/GltfDocument.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {

int failures = 0;

void expect(bool condition, std::string_view message) {
    if (condition) {
        return;
    }

    ++failures;
    std::cerr << "[fail] " << message << '\n';
}

novacore::assets::GltfMeshData makeMesh() {
    novacore::assets::GltfPrimitiveData primitive{};
    primitive.positions = {
        {-0.5F, 0.0F, 0.0F},
        {0.5F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F},
    };
    primitive.normals = {
        {0.0F, 1.0F, 0.0F},
        {0.0F, 1.0F, 0.0F},
        {0.0F, 1.0F, 0.0F},
    };
    primitive.texcoords = {
        {0.0F, 0.0F},
        {1.0F, 0.0F},
        {0.5F, 1.0F},
    };
    primitive.indices = {0, 1, 2};

    novacore::assets::GltfMeshData mesh{};
    mesh.primitives.push_back(std::move(primitive));
    return mesh;
}

nemisis::dev::MeshResourceLookup registerSceneMeshes(novacore::render::Renderer& renderer) {
    static constexpr std::array<std::string_view, 19> kSceneMeshes{
        "env_test_arena_kit_01",
        "prop_target_dummy_01",
        "chr_dev_soldier_a",
        "chr_proto_humanoid_01",
        "chr_a1_stylized_operator_01",
        "map_floor_tile_01",
        "map_wall_panel_01",
        "map_cover_crate_01",
        "map_ramp_01",
        "map_target_stand_01",
        "wpn_ar_01",
        "wpn_smg_01",
        "wpn_sidearm_01",
        "wpn_proto_smg_01",
        "wpn_a1_compact_rifle_01",
        "wpn_a1_modern_rifle_01",
        "wpn_a1_compact_sidearm_01",
        "chr_dev_arms_a",
        "chr_a1_fp_arms_01",
    };

    nemisis::dev::MeshResourceLookup lookup;
    const auto mesh = makeMesh();
    for (const auto assetId : kSceneMeshes) {
        lookup.emplace(std::string(assetId), renderer.registerMeshResource(std::string(assetId), mesh));
    }
    return lookup;
}

void testDevRangeRenderSceneBuildsExpectedSubmissions() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    nemisis::dev::DebugTargetState target{};
    target.eliminated = false;

    nemisis::dev::DevRangePlayerRenderState player{};
    player.position = {1.0F, 0.0F, 2.0F};
    player.view.yawDegrees = 32.0F;
    player.view.pitchDegrees = -7.0F;
    player.hasMovementState = true;
    nemisis::dev::GreyboxCollisionResult collision{};
    collision.grounded = true;
    collision.groundPrimitiveId = "floor_main";
    collision.groundNormal = {0.0F, 1.0F, 0.0F};

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{
            &world,
            &target,
            &collision,
            &lookup,
            player,
        });

    expect(frame.camera3D.enabled, "dev range render scene enables 3D camera");
    expect(frame.camera3D.position.x == 1.0F, "dev range render scene copies player x");
    expect(frame.camera3D.position.y > 1.6F && frame.camera3D.position.y < 1.7F, "dev range camera uses eye height");
    expect(frame.camera3D.position.z == 2.0F, "dev range render scene copies player z");
    expect(frame.camera3D.yawDegrees == 32.0F, "dev range render scene copies yaw");
    expect(frame.camera3D.pitchDegrees == -7.0F, "dev range render scene copies pitch");
    expect(frame.camera3D.verticalFovDegrees == 74.0F, "dev range render scene uses default FOV");
    expect(frame.lighting.ambientIntensity > 0.37F && frame.lighting.ambientIntensity < 0.39F, "dev range render scene applies lighting profile");
    expect(frame.lighting.sunDirection.y > 0.80F, "dev range lighting points from above");

    expect(!world.primitives.empty(), "greybox world fixture has primitives");
    expect(stats.worldBoxCount == world.primitives.size() + 9U, "dev range render scene emits world, weapon, hands, muzzle, and aim boxes");
    expect(frame.worldBoxes.size() == stats.worldBoxCount, "world box count matches frame");
    expect(stats.meshInstanceCount == 14, "dev range render scene emits static and first-person mesh instances");
    expect(frame.worldMeshes.size() == 14, "frame receives all mesh instances");
    expect(stats.skippedMeshInstanceCount == 0, "dev range render scene skips no mesh when lookup is complete");
    expect(stats.firstPersonMeshCount == 2, "dev range render scene emits weapon and arms first-person mesh anchors");
    expect(stats.aimMarkerBoxCount == 5, "dev range render scene emits five aim marker boxes");
    expect(stats.worldLineCount == 2, "dev range render scene emits aim and ground-normal lines");
    expect(frame.worldLines.size() == 2, "frame receives world debug lines");

    const auto firstMesh = frame.worldMeshes.front();
    expect(firstMesh.assetId == "env_test_arena_kit_01", "first dev mesh is the arena kit");
    expect(firstMesh.mesh.isValid(), "first dev mesh has a valid renderer resource handle");
}

void testDevRangeRenderSceneCountsMissingMeshHandles() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    lookup.erase("wpn_a1_compact_rifle_01");
    lookup.erase("wpn_ar_01");
    lookup.erase("chr_a1_fp_arms_01");
    lookup.erase("chr_dev_arms_a");

    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    nemisis::dev::DebugTargetState target{};
    target.eliminated = true;

    nemisis::dev::DevRangePlayerRenderState player{};
    player.position = world.playerSpawn;
    player.view.yawDegrees = 180.0F;

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{
            &world,
            &target,
            nullptr,
            &lookup,
            player,
        });

    expect(stats.meshInstanceCount == 12, "dev range render scene still emits available static meshes");
    expect(frame.worldMeshes.size() == 12, "frame mesh count drops missing handles");
    expect(stats.skippedMeshInstanceCount == 4, "dev range render scene counts missing primary and fallback handles");
    expect(stats.firstPersonMeshCount == 0, "first-person mesh stats reflect missing weapon/arms handles");
    expect(stats.worldLineCount == 1, "dev range render scene still emits aim line without collision sample");
}

void testDevRangeRenderSceneHandlesMissingInputs() {
    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(frame, {});

    expect(!frame.camera3D.enabled, "dev range render scene leaves camera disabled without world/input");
    expect(frame.worldBoxes.empty(), "dev range render scene emits no boxes without world/input");
    expect(frame.worldMeshes.empty(), "dev range render scene emits no meshes without world/input");
    expect(stats.worldBoxCount == 0, "dev range render scene reports zero boxes without world/input");
    expect(stats.meshInstanceCount == 0, "dev range render scene reports zero meshes without world/input");
}

void testDevRangeRenderSceneCanDisableDebugLines() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    nemisis::dev::DebugTargetState target{};
    nemisis::dev::GreyboxCollisionResult collision{};
    collision.grounded = true;
    collision.groundNormal = {0.0F, 1.0F, 0.0F};

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{
            &world,
            &target,
            &collision,
            &lookup,
            {},
            {},
            false,
        });

    expect(stats.worldLineCount == 0, "dev range render scene can disable world debug lines");
    expect(frame.worldLines.empty(), "frame receives no world lines when disabled");
}

} // namespace

int main() {
    testDevRangeRenderSceneBuildsExpectedSubmissions();
    testDevRangeRenderSceneCountsMissingMeshHandles();
    testDevRangeRenderSceneHandlesMissingInputs();
    testDevRangeRenderSceneCanDisableDebugLines();

    if (failures > 0) {
        std::cerr << failures << " dev range render scene test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Dev range render scene tests passed\n";
    return EXIT_SUCCESS;
}
