#include "nemisis/assets/DevAssetBindings.hpp"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

int failures = 0;

struct AssetSpec final {
    std::string id;
    std::string kind;
    std::string category;
    std::string source;
    std::string cooked;
    std::string collision = "none";
};

void expect(bool condition, std::string_view message) {
    if (condition) {
        return;
    }

    ++failures;
    std::cerr << "[fail] " << message << '\n';
}

std::vector<AssetSpec> requiredSpecs() {
    return {
        {"prop_target_dummy_01", "mesh", "prop", "assets/source/blender/props/prop_target_dummy_01.blend", "assets/export/gltf/props/prop_target_dummy_01.glb", "col_prop_target_dummy_01"},
        {"wpn_ar_01", "mesh", "weapon", "assets/source/blender/weapons/wpn_ar_01.blend", "assets/export/gltf/weapons/wpn_ar_01.glb"},
        {"wpn_smg_01", "mesh", "weapon", "assets/source/blender/weapons/wpn_smg_01.blend", "assets/export/gltf/weapons/wpn_smg_01.glb"},
        {"wpn_sidearm_01", "mesh", "weapon", "assets/source/blender/weapons/wpn_sidearm_01.blend", "assets/export/gltf/weapons/wpn_sidearm_01.glb"},
        {"env_test_arena_kit_01", "scene", "environment", "assets/source/blender/environments/env_test_arena_kit_01.blend", "assets/export/gltf/environments/env_test_arena_kit_01.glb", "per_piece_col_prefix"},
        {"chr_player_capsule_proxy_01", "mesh", "character", "assets/source/blender/characters/chr_player_capsule_proxy_01.blend", "assets/export/gltf/characters/chr_player_capsule_proxy_01.glb", "col_chr_player_capsule_proxy_01"},
        {"chr_dev_arms_a", "mesh", "character", "assets/source/blender/characters/chr_dev_arms_a.blend", "assets/export/gltf/characters/chr_dev_arms_a.glb"},
        {"chr_dev_soldier_a", "mesh", "character", "assets/source/blender/characters/chr_dev_soldier_a.blend", "assets/export/gltf/characters/chr_dev_soldier_a.glb", "col_chr_dev_soldier_a_capsule"},
    };
}

void writeManifest(const std::filesystem::path& path, std::span<const AssetSpec> specs) {
    std::ofstream file(path);
    file << R"({
        "manifest": { "name": "dev_asset_bindings_test", "root": "assets" },
        "assets": [
)";
    for (std::size_t index = 0; index < specs.size(); ++index) {
        const auto& spec = specs[index];
        file << R"({
                "id": ")" << spec.id << R"(",
                "kind": ")" << spec.kind << R"(",
                "source": ")" << spec.source << R"(",
                "cooked": ")" << spec.cooked << R"(",
                "streamable": true,
                "priority": 90,
                "estimated_bytes": 1024,
                "tags": ["dev_sandbox"]
            })";
        if (index + 1 < specs.size()) {
            file << ',';
        }
        file << '\n';
    }
    file << R"(]
    })";
}

void writeMetadata(const std::filesystem::path& root, const AssetSpec& spec) {
    auto metadataPath = root / spec.cooked;
    metadataPath.replace_extension(".metadata.json");
    std::filesystem::create_directories(metadataPath.parent_path());

    std::ofstream file(metadataPath);
    file << R"({
        "id": ")" << spec.id << R"(",
        "source": ")" << spec.source << R"(",
        "export": ")" << spec.cooked << R"(",
        "category": ")" << spec.category << R"(",
        "scale_meters": true,
        "runtime_up_axis": "Y",
        "gameplay_forward_axis": "+Z",
        "sockets": ["socket_root"],
        "collision": ")" << spec.collision << R"(",
        "lods": [")" << spec.id << R"(_lod0"],
        "license": "original_project_asset",
        "generated_by": "dev_asset_bindings_tests"
    })";
}

void testRequiredDevAssetsBindToMeshCatalog() {
    const auto root = std::filesystem::temp_directory_path() / "nemisis_dev_asset_bindings_ready";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const auto specs = requiredSpecs();
    const auto manifestPath = root / "assets.json";
    writeManifest(manifestPath, specs);
    for (const auto& spec : specs) {
        writeMetadata(root, spec);
    }

    nemisis::assets::GameAssetCatalog catalog;
    const auto loadResult = catalog.loadFromJson(manifestPath);
    expect(loadResult.ok(), "asset catalog loads required dev assets");

    nemisis::assets::DevAssetBindings bindings;
    const auto summary = bindings.bindRequiredAssets(catalog, root);
    expect(summary.ready(), "required dev assets bind cleanly");
    expect(summary.requiredAssetCount == nemisis::assets::requiredDevSandboxRenderableAssetIds().size(), "summary counts required assets");
    expect(summary.renderableAssetCount == summary.requiredAssetCount, "all required assets become mesh handles");
    expect(summary.metadataAssetCount == summary.requiredAssetCount, "all required assets load metadata");
    expect(bindings.meshCatalog().contains("wpn_ar_01"), "mesh catalog contains AR handle");
    expect(bindings.meshCatalog().contains("env_test_arena_kit_01"), "mesh catalog contains arena scene handle");

    std::filesystem::remove_all(root);
}

void testMissingMetadataIsReported() {
    const auto root = std::filesystem::temp_directory_path() / "nemisis_dev_asset_bindings_missing";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const std::array<AssetSpec, 1> specs{
        AssetSpec{"wpn_ar_01", "mesh", "weapon", "assets/source/blender/weapons/wpn_ar_01.blend", "assets/export/gltf/weapons/wpn_ar_01.glb"},
    };
    const auto manifestPath = root / "assets.json";
    writeManifest(manifestPath, specs);

    nemisis::assets::GameAssetCatalog catalog;
    const auto loadResult = catalog.loadFromJson(manifestPath);
    expect(loadResult.ok(), "asset catalog loads incomplete dev asset fixture");

    const std::array<std::string_view, 1> ids{"wpn_ar_01"};
    nemisis::assets::DevAssetBindings bindings;
    const auto summary = bindings.bindAssets(catalog, ids, root);
    expect(!summary.ready(), "missing metadata prevents ready state");
    expect(summary.renderableAssetCount == 0, "missing metadata is not registered");
    expect(!summary.errors.empty(), "missing metadata reports error");

    std::filesystem::remove_all(root);
}

} // namespace

int main() {
    testRequiredDevAssetsBindToMeshCatalog();
    testMissingMetadataIsReported();

    if (failures > 0) {
        std::cerr << failures << " dev asset binding test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis dev asset binding tests passed\n";
    return EXIT_SUCCESS;
}
