#include "nemisis/assets/DevAssetBindings.hpp"

#include "novacore/assets/GltfDocument.hpp"
#include "novacore/assets/GltfMetadata.hpp"

#include <string>
#include <utility>

namespace nemisis::assets {

namespace {

constexpr RequiredDevAssetIds kRequiredDevAssets{
    "prop_target_dummy_01",
    "wpn_ar_01",
    "wpn_smg_01",
    "wpn_sidearm_01",
    "env_test_arena_kit_01",
    "chr_player_capsule_proxy_01",
    "chr_dev_arms_a",
    "chr_dev_soldier_a",
    "wpn_proto_smg_01",
    "chr_proto_humanoid_01",
    "map_wall_panel_01",
    "map_floor_tile_01",
    "map_cover_crate_01",
    "map_ramp_01",
    "map_target_stand_01",
    "wpn_a1_compact_rifle_01",
    "wpn_a1_modern_rifle_01",
    "wpn_a1_compact_sidearm_01",
    "chr_a1_stylized_operator_01",
    "chr_a1_fp_arms_01",
    "wpn_a2_blackout_carbine_01",
    "wpn_a2_modular_rifle_01",
    "wpn_a2_striker_sidearm_01",
    "chr_a2_pilot_operator_01",
    "map_a2_wallrun_panel_01",
    "map_a2_slide_ramp_01",
    "map_a2_cover_crate_01",
    "prop_a2_range_hero_01",
};

void appendErrors(
    DevAssetBindingSummary& summary,
    std::string_view assetId,
    const std::vector<std::string>& errors) {
    for (const auto& error : errors) {
        summary.errors.push_back(std::string(assetId) + ": " + error);
    }
}

} // namespace

const RequiredDevAssetIds& requiredDevSandboxRenderableAssetIds() {
    return kRequiredDevAssets;
}

void DevAssetBindings::clear() {
    meshes_.clear();
}

DevAssetBindingSummary DevAssetBindings::bindRequiredAssets(
    const GameAssetCatalog& catalog,
    const std::filesystem::path& runtimeRoot) {
    return bindAssets(catalog, kRequiredDevAssets, runtimeRoot);
}

DevAssetBindingSummary DevAssetBindings::bindAssets(
    const GameAssetCatalog& catalog,
    std::span<const std::string_view> assetIds,
    const std::filesystem::path& runtimeRoot) {
    clear();

    DevAssetBindingSummary summary{};
    summary.requiredAssetCount = assetIds.size();

    for (const auto assetId : assetIds) {
        const auto* record = catalog.find(assetId);
        if (record == nullptr) {
            ++summary.missingAssetCount;
            summary.errors.push_back("Missing required dev asset: " + std::string(assetId));
            continue;
        }

        const auto recordErrors = novacore::render::validateRenderableAssetRecord(*record);
        if (!recordErrors.empty()) {
            appendErrors(summary, assetId, recordErrors);
            continue;
        }

        novacore::assets::GltfAssetMetadata metadata{};
        const auto metadataPath = runtimeRoot / novacore::assets::metadataPathForCookedAsset(*record);
        const auto metadataResult = novacore::assets::loadGltfAssetMetadataFromJson(metadataPath, metadata);
        if (!metadataResult.ok()) {
            appendErrors(summary, assetId, metadataResult.errors);
            continue;
        }

        const auto validationErrors = novacore::assets::validateGltfAssetMetadata(*record, metadata);
        if (!validationErrors.empty()) {
            appendErrors(summary, assetId, validationErrors);
            continue;
        }
        ++summary.metadataAssetCount;

        novacore::assets::GltfMeshData meshData{};
        const auto meshDataPath = runtimeRoot / record->cookedPath;
        const auto meshDataResult = novacore::assets::loadGltfMeshData(meshDataPath, meshData);
        if (!meshDataResult.ok()) {
            appendErrors(summary, assetId, meshDataResult.errors);
            continue;
        }

        const auto meshCount = meshData.sceneInfo.meshCount;
        const auto nodeCount = meshData.sceneInfo.nodeCount;
        const auto materialCount = meshData.sceneInfo.materialCount;
        const auto binaryBytes = meshData.sceneInfo.binaryBytes;
        const auto primitiveCount = meshData.primitiveCount();
        const auto vertexCount = meshData.vertexCount();
        const auto indexCount = meshData.indexCount();

        const auto handle = meshes_.registerImportedGltfAsset(*record, std::move(metadata), std::move(meshData));
        if (!handle.isValid()) {
            summary.errors.push_back("Failed to register dev mesh asset: " + std::string(assetId));
            continue;
        }

        ++summary.importedAssetCount;
        ++summary.extractedAssetCount;
        summary.totalMeshCount += meshCount;
        summary.totalNodeCount += nodeCount;
        summary.totalMaterialCount += materialCount;
        summary.totalBinaryBytes += binaryBytes;
        summary.totalPrimitiveCount += primitiveCount;
        summary.totalVertexCount += vertexCount;
        summary.totalIndexCount += indexCount;
    }

    summary.renderableAssetCount = meshes_.meshCount();
    return summary;
}

const novacore::render::MeshCatalog& DevAssetBindings::meshCatalog() const {
    return meshes_;
}

std::size_t DevAssetBindings::meshCount() const {
    return meshes_.meshCount();
}

} // namespace nemisis::assets
