#include "nemisis/dev/DevRangeRenderScene.hpp"
#include "nemisis/dev/GreyboxWorld.hpp"

#include "novacore/assets/GltfDocument.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <optional>
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
    static constexpr std::array<std::string_view, 35> kSceneMeshes{
        "env_test_arena_kit_01",
        "env_project_skybox1",
        "prop_target_dummy_01",
        "chr_dev_soldier_a",
        "chr_project_male1",
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
        "wpn_a2_blackout_carbine_01",
        "wpn_a2_modular_rifle_01",
        "wpn_a2_striker_sidearm_01",
        "chr_a2_pilot_operator_01",
        "map_a2_wallrun_panel_01",
        "map_a2_slide_ramp_01",
        "map_a2_cover_crate_01",
        "prop_a2_range_hero_01",
        "wpn_project_rifle_m4a1",
        "wpn_project_rifle_afr120",
        "wpn_project_rifle_ncar",
        "wpn_project_smg_fr17",
        "wpn_project_sidearm_glock19",
        "wpn_project_sidearm_p320",
    };

    nemisis::dev::MeshResourceLookup lookup;
    const auto mesh = makeMesh();
    for (const auto assetId : kSceneMeshes) {
        lookup.emplace(std::string(assetId), renderer.registerMeshResource(std::string(assetId), mesh));
    }
    return lookup;
}

std::optional<novacore::render::RenderMesh3D> findMesh(
    const novacore::render::RenderFrameInfo& frame,
    std::string_view assetId) {
    for (const auto& mesh : frame.worldMeshes) {
        if (mesh.assetId == assetId) {
            return mesh;
        }
    }
    return std::nullopt;
}

std::optional<novacore::render::RenderMesh3D> findLastMesh(
    const novacore::render::RenderFrameInfo& frame,
    std::string_view assetId) {
    for (auto it = frame.worldMeshes.rbegin(); it != frame.worldMeshes.rend(); ++it) {
        if (it->assetId == assetId) {
            return *it;
        }
    }
    return std::nullopt;
}

void testDevRangeRenderSceneBuildsExpectedSubmissions() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();

    nemisis::dev::DevRangePlayerRenderState player{};
    player.position = {1.0F, 0.0F, 2.0F};
    player.view.yawDegrees = 32.0F;
    player.view.pitchDegrees = -7.0F;
    player.hasMovementState = true;
    nemisis::dev::GreyboxCollisionResult collision{};
    collision.grounded = true;
    collision.groundPrimitiveId = "floor_main";
    collision.groundNormal = {0.0F, 1.0F, 0.0F};
    collision.contacts.push_back(nemisis::dev::GreyboxContact{
        "floor_main",
        nemisis::dev::GreyboxPrimitiveKind::Floor,
        nemisis::dev::GreyboxContactRole::Ground,
        {1.0F, 0.0F, 2.0F},
        {0.0F, 1.0F, 0.0F},
        0.0F,
        1.0F,
        0.0F,
        false,
        true,
    });

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{
            &world,
            &targetRange,
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
    expect(frame.lighting.ambientIntensity > 0.33F && frame.lighting.ambientIntensity < 0.35F, "dev range render scene applies lighting profile");
    expect(frame.lighting.sunDirection.y > 0.80F, "dev range lighting points from above");
    expect(frame.lighting.fillIntensity > 0.20F, "dev range lighting applies fill light for imported meshes");
    expect(frame.lighting.rimIntensity > 0.20F, "dev range lighting applies rim light for mesh readability");

    expect(!world.primitives.empty(), "greybox world fixture has primitives");
    expect(stats.worldBoxCount == (world.primitives.size() - 1U) + targetRange.lanes.size() + 19U, "dev range render scene emits world, lane, asset stage, pickup pads, muzzle, mantle/tech, and aim boxes");
    expect(frame.worldBoxes.size() == stats.worldBoxCount, "world box count matches frame");
    expect(stats.meshInstanceCount == 37, "dev range render scene emits skybox, static, player body, target lane, imported Project asset, A2 showcase, and first-person mesh instances");
    expect(frame.worldMeshes.size() == 37, "frame receives all mesh instances");
    expect(stats.skippedMeshInstanceCount == 0, "dev range render scene skips no mesh when lookup is complete");
    expect(stats.firstPersonMeshCount == 3, "dev range render scene emits weapon, camera-linked body, and arms first-person mesh anchors");
    expect(stats.targetMeshCount == targetRange.lanes.size(), "dev range render scene emits one actor mesh per target lane");
    expect(stats.aimMarkerBoxCount == 5, "dev range render scene emits five aim marker boxes");
    expect(stats.worldLineCount == 3, "dev range render scene emits aim, ground-normal, and contact lines");
    expect(frame.worldLines.size() == 3, "frame receives world debug lines");

    const auto firstMesh = frame.worldMeshes.front();
    expect(firstMesh.assetId == "env_project_skybox1", "first dev mesh is the project skybox background");
    expect(firstMesh.mesh.isValid(), "first dev mesh has a valid renderer resource handle");
    expect(findMesh(frame, "env_test_arena_kit_01").has_value(), "dev range render scene still submits the arena kit");

    const auto firstPersonRifle = findLastMesh(frame, "wpn_project_rifle_m4a1");
    expect(firstPersonRifle.has_value(), "first-person Project rifle mesh is submitted");
    if (firstPersonRifle.has_value()) {
        expect(firstPersonRifle->position.z > player.position.z + 0.45F, "first-person rifle sits in front of the camera");
        expect(firstPersonRifle->yawDegrees > 205.0F && firstPersonRifle->yawDegrees < 215.0F, "first-person rifle applies 180-degree imported-asset yaw correction");
        expect(firstPersonRifle->rollDegrees < -85.0F && firstPersonRifle->rollDegrees > -95.0F, "first-person rifle applies imported-asset roll correction");
        expect(firstPersonRifle->scale.x > 1.8F, "first-person rifle uses a visible weapon scale");
    }
}

void testDevRangeRenderSceneMovesWeaponTowardSightlineInAds() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();

    nemisis::dev::DevRangePlayerRenderState hipPlayer{};
    hipPlayer.position = world.playerSpawn;
    hipPlayer.view.yawDegrees = 0.0F;
    hipPlayer.view.pitchDegrees = 0.0F;
    hipPlayer.hasMovementState = true;
    hipPlayer.activeWeaponId = "ar_01";
    hipPlayer.activeWeaponClass = nemisis::weapons::WeaponClass::AssaultRifle;

    auto adsPlayer = hipPlayer;
    adsPlayer.adsAlpha = 1.0F;

    novacore::render::RenderFrameInfo hipFrame{};
    (void)nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        hipFrame,
        nemisis::dev::DevRangeRenderSceneDesc{&world, &targetRange, nullptr, &lookup, hipPlayer});

    novacore::render::RenderFrameInfo adsFrame{};
    (void)nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        adsFrame,
        nemisis::dev::DevRangeRenderSceneDesc{&world, &targetRange, nullptr, &lookup, adsPlayer});

    const auto hipRifle = findLastMesh(hipFrame, "wpn_project_rifle_m4a1");
    const auto adsRifle = findLastMesh(adsFrame, "wpn_project_rifle_m4a1");
    expect(hipRifle.has_value() && adsRifle.has_value(), "hip and ADS frames submit first-person rifle");
    if (hipRifle.has_value() && adsRifle.has_value()) {
        expect(std::abs(adsRifle->position.x) < std::abs(hipRifle->position.x), "ADS pulls rifle closer to camera centerline");
        expect(adsRifle->position.z < hipRifle->position.z, "ADS pulls rifle slightly closer to the player");
        expect(adsRifle->position.y > hipRifle->position.y, "ADS raises rifle toward eye line");
    }
}

void testDevRangeRenderSceneUsesPerWeaponImportAxisCorrections() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();

    nemisis::dev::DevRangePlayerRenderState smgPlayer{};
    smgPlayer.position = world.playerSpawn;
    smgPlayer.view.yawDegrees = 0.0F;
    smgPlayer.view.pitchDegrees = 0.0F;
    smgPlayer.activeWeaponId = "smg_01";
    smgPlayer.activeWeaponClass = nemisis::weapons::WeaponClass::Smg;

    novacore::render::RenderFrameInfo smgFrame{};
    (void)nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        smgFrame,
        nemisis::dev::DevRangeRenderSceneDesc{&world, &targetRange, nullptr, &lookup, smgPlayer});

    const auto smgMesh = findLastMesh(smgFrame, "wpn_project_smg_fr17");
    expect(smgMesh.has_value(), "SMG first-person Project mesh is submitted");
    if (smgMesh.has_value()) {
        expect(smgMesh->yawDegrees > -1.0F && smgMesh->yawDegrees < 1.0F, "SMG uses its own forward-axis correction instead of AR yaw");
        expect(smgMesh->rollDegrees < -85.0F && smgMesh->rollDegrees > -95.0F, "SMG uses long-weapon upright roll correction");
    }

    auto sidearmPlayer = smgPlayer;
    sidearmPlayer.activeWeaponId = "sidearm_01";
    sidearmPlayer.activeWeaponClass = nemisis::weapons::WeaponClass::Sidearm;

    novacore::render::RenderFrameInfo sidearmFrame{};
    (void)nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        sidearmFrame,
        nemisis::dev::DevRangeRenderSceneDesc{&world, &targetRange, nullptr, &lookup, sidearmPlayer});

    const auto sidearmMesh = findLastMesh(sidearmFrame, "wpn_project_sidearm_glock19");
    expect(sidearmMesh.has_value(), "sidearm first-person Project mesh is submitted");
    if (sidearmMesh.has_value()) {
        expect(sidearmMesh->yawDegrees > -1.0F && sidearmMesh->yawDegrees < 1.0F, "sidearm uses non-AR forward-axis correction");
        expect(sidearmMesh->rollDegrees > 85.0F && sidearmMesh->rollDegrees < 95.0F, "sidearm keeps pistol upright roll correction");
    }
}

void testDevRangeRenderScenePlacesA2AssetsInSpawnView() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();

    nemisis::dev::DevRangePlayerRenderState player{};
    player.position = world.playerSpawn;
    player.view.yawDegrees = 0.0F;
    player.view.pitchDegrees = 0.0F;

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{
            &world,
            &targetRange,
            nullptr,
            &lookup,
            player,
        });

    expect(stats.meshInstanceCount == 36, "spawn view scene emits every expected mesh without local body");
    expect(stats.skippedMeshInstanceCount == 0, "spawn view scene has no skipped A2 meshes");

    const auto operatorMesh = findMesh(frame, "chr_a2_pilot_operator_01");
    const auto carbineMesh = findMesh(frame, "wpn_a2_blackout_carbine_01");
    const auto rifleMesh = findMesh(frame, "wpn_a2_modular_rifle_01");
    const auto sidearmMesh = findMesh(frame, "wpn_a2_striker_sidearm_01");
    const auto heroMesh = findMesh(frame, "prop_a2_range_hero_01");
    const auto projectCharacterMesh = findMesh(frame, "chr_project_male1");
    const auto projectSkyboxMesh = findMesh(frame, "env_project_skybox1");
    const auto projectRifleMesh = findMesh(frame, "wpn_project_rifle_m4a1");
    const auto projectSmgMesh = findMesh(frame, "wpn_project_smg_fr17");
    const auto projectSidearmMesh = findMesh(frame, "wpn_project_sidearm_glock19");

    expect(operatorMesh.has_value(), "A2 operator mesh is submitted");
    expect(carbineMesh.has_value(), "A2 carbine mesh is submitted");
    expect(rifleMesh.has_value(), "A2 rifle mesh is submitted");
    expect(sidearmMesh.has_value(), "A2 sidearm mesh is submitted");
    expect(heroMesh.has_value(), "A2 hero prop mesh is submitted");
    expect(projectCharacterMesh.has_value(), "Project character mesh is submitted");
    expect(projectSkyboxMesh.has_value(), "Project skybox mesh is submitted");
    expect(projectRifleMesh.has_value(), "Project rifle mesh is submitted");
    expect(projectSmgMesh.has_value(), "Project SMG mesh is submitted");
    expect(projectSidearmMesh.has_value(), "Project sidearm mesh is submitted");

    if (operatorMesh.has_value()) {
        expect(operatorMesh->position.z > world.playerSpawn.z + 5.0F && operatorMesh->position.z < world.playerSpawn.z + 8.5F, "A2 operator starts in front of spawn");
        expect(operatorMesh->position.x > -4.0F && operatorMesh->position.x < -1.0F, "A2 operator is inside initial horizontal view");
        expect(operatorMesh->scale.y > 1.0F, "A2 operator is scaled for visible review");
    }
    if (carbineMesh.has_value() && rifleMesh.has_value() && sidearmMesh.has_value()) {
        expect(carbineMesh->position.y > 1.0F, "A2 carbine is raised on the review rack");
        expect(rifleMesh->position.y > 1.0F, "A2 rifle is raised on the review rack");
        expect(sidearmMesh->position.y > 1.0F, "A2 sidearm is raised on the review rack");
        expect(carbineMesh->position.z > world.playerSpawn.z + 4.5F && sidearmMesh->position.z > world.playerSpawn.z + 4.5F, "A2 weapons start in front of spawn");
    }
    if (heroMesh.has_value()) {
        expect(heroMesh->position.z > world.playerSpawn.z + 7.5F, "A2 hero prop anchors the visible asset stage");
    }
    if (projectSkyboxMesh.has_value()) {
        expect(projectSkyboxMesh->scale.x > 30.0F, "Project skybox is expanded around the range camera");
    }
    if (projectCharacterMesh.has_value() && projectRifleMesh.has_value() && projectSmgMesh.has_value() && projectSidearmMesh.has_value()) {
        expect(
            std::any_of(
                frame.worldMeshes.begin(),
                frame.worldMeshes.end(),
                [&world](const novacore::render::RenderMesh3D& mesh) {
                    return mesh.assetId == "chr_project_male1" &&
                        mesh.position.z > world.playerSpawn.z + 5.0F &&
                        mesh.position.x < -6.5F;
                }),
            "Project character starts in the visible asset stage");
        expect(projectRifleMesh->position.y > 0.9F, "Project rifle is raised on the review rack");
        expect(projectSmgMesh->position.y > 0.9F, "Project SMG is raised on the review rack");
        expect(projectSidearmMesh->position.y > 0.8F, "Project sidearm is raised on the review rack");
    }
}

void testDevRangeRenderSceneCountsMissingMeshHandles() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    lookup.erase("wpn_project_rifle_m4a1");
    lookup.erase("wpn_a2_modular_rifle_01");
    lookup.erase("chr_a1_fp_arms_01");
    lookup.erase("chr_dev_arms_a");

    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();
    targetRange.lanes[1].target.eliminated = true;

    nemisis::dev::DevRangePlayerRenderState player{};
    player.position = world.playerSpawn;
    player.view.yawDegrees = 180.0F;

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{
            &world,
            &targetRange,
            nullptr,
            &lookup,
            player,
        });

    expect(stats.meshInstanceCount == 32, "dev range render scene still emits available skybox, static, A2, Project, target lane, and character-proxy arms meshes");
    expect(frame.worldMeshes.size() == 32, "frame mesh count drops missing weapon handles but keeps character-proxy arms");
    expect(stats.skippedMeshInstanceCount == 6, "dev range render scene counts missing showcase, primary, fallback, and first-person arm handles");
    expect(stats.firstPersonMeshCount == 1, "first-person mesh stats keep character-proxy arms when weapon handles are missing");
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
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();
    nemisis::dev::GreyboxCollisionResult collision{};
    collision.grounded = true;
    collision.groundNormal = {0.0F, 1.0F, 0.0F};

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{
            &world,
            &targetRange,
            &collision,
            &lookup,
            {},
            {},
            false,
        });

    expect(stats.worldLineCount == 0, "dev range render scene can disable world debug lines");
    expect(frame.worldLines.empty(), "frame receives no world lines when disabled");
}

void testDevRangeRenderSceneDrawsMantleCandidateDebugLines() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();
    nemisis::dev::GreyboxCollisionResult collision{};
    collision.mantleCandidate = true;
    collision.mantleObstaclePoint = {3.8F, 1.3F, -7.5F};
    collision.mantleTargetPosition = {3.8F, 1.3F, -6.95F};
    collision.mantlePrimitiveId = "ledge_training_mid";

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{
            &world,
            &targetRange,
            &collision,
            &lookup,
            {},
        });

    expect(stats.worldLineCount == 3, "dev range render scene emits aim and mantle candidate lines");
    expect(frame.worldLines.size() == 3, "frame receives mantle candidate world lines");
}

void testDevRangeRenderSceneDrawsSweepDebugLines() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();
    nemisis::dev::GreyboxCollisionResult collision{};
    collision.swept = true;
    collision.sweepHit = true;
    collision.sweepStartPosition = {3.8F, 0.0F, -10.0F};
    collision.requestedDisplacement = {0.0F, 0.0F, 7.0F};
    collision.appliedDisplacement = {0.0F, 0.0F, 2.05F};
    collision.sweepNormal = {0.0F, 0.0F, -1.0F};
    collision.sweepPrimitiveId = "ledge_training_mid";
    collision.contacts.push_back(nemisis::dev::GreyboxContact{
        "ledge_training_mid",
        nemisis::dev::GreyboxPrimitiveKind::Ledge,
        nemisis::dev::GreyboxContactRole::Sweep,
        {3.8F, 0.16F, -7.95F},
        {0.0F, 0.0F, -1.0F},
        0.0F,
        0.42F,
        0.0F,
        true,
        false,
    });

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{
            &world,
            &targetRange,
            &collision,
            &lookup,
            {},
        });

    expect(stats.worldLineCount == 5, "dev range render scene emits aim, sweep, and contact debug lines");
    expect(frame.worldLines.size() == 5, "frame receives sweep debug world lines");
}

void testDevRangeRenderSceneDrawsContactDebugLines() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();
    nemisis::dev::GreyboxCollisionResult collision{};
    collision.contacts.push_back(nemisis::dev::GreyboxContact{
        "step_training_low",
        nemisis::dev::GreyboxPrimitiveKind::Cover,
        nemisis::dev::GreyboxContactRole::Step,
        {-3.5F, 0.36F, -7.0F},
        {0.0F, 1.0F, 0.0F},
        0.0F,
        1.0F,
        0.0F,
        false,
        true,
    });
    collision.contacts.push_back(nemisis::dev::GreyboxContact{
        "wallrun_left_panel_a",
        nemisis::dev::GreyboxPrimitiveKind::WallRunPanel,
        nemisis::dev::GreyboxContactRole::Wall,
        {-18.0F, 1.0F, -5.5F},
        {1.0F, 0.0F, 0.0F},
        0.0F,
        1.0F,
        0.0F,
        true,
        false,
    });

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{
            &world,
            &targetRange,
            &collision,
            &lookup,
            {},
        });

    expect(stats.worldLineCount == 3, "dev range render scene emits aim plus two contact lines");
    expect(frame.worldLines.size() == 3, "frame receives contact debug world lines");
}

} // namespace

int main() {
    testDevRangeRenderSceneBuildsExpectedSubmissions();
    testDevRangeRenderSceneMovesWeaponTowardSightlineInAds();
    testDevRangeRenderSceneUsesPerWeaponImportAxisCorrections();
    testDevRangeRenderScenePlacesA2AssetsInSpawnView();
    testDevRangeRenderSceneCountsMissingMeshHandles();
    testDevRangeRenderSceneHandlesMissingInputs();
    testDevRangeRenderSceneCanDisableDebugLines();
    testDevRangeRenderSceneDrawsMantleCandidateDebugLines();
    testDevRangeRenderSceneDrawsSweepDebugLines();
    testDevRangeRenderSceneDrawsContactDebugLines();

    if (failures > 0) {
        std::cerr << failures << " dev range render scene test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Dev range render scene tests passed\n";
    return EXIT_SUCCESS;
}
