#include "nemisis/assets/GameAssetCatalog.hpp"

#include <utility>

namespace nemisis::assets {

GameAssetCatalogLoadResult GameAssetCatalog::loadFromJson(const std::filesystem::path& path) {
    novacore::assets::AssetManifest manifest;
    const auto result = novacore::assets::loadAssetManifestFromJson(path, manifest);
    if (!result.ok()) {
        return GameAssetCatalogLoadResult{result.errors};
    }

    manifest_ = std::move(manifest);
    registry_.clear();
    registry_.mountManifest(manifest_);
    return GameAssetCatalogLoadResult{};
}

void GameAssetCatalog::clear() {
    manifest_.clear();
    registry_.clear();
}

const novacore::assets::AssetRecord* GameAssetCatalog::find(std::string_view id) const {
    return registry_.find(id);
}

bool GameAssetCatalog::contains(std::string_view id) const {
    return registry_.contains(id);
}

std::vector<novacore::assets::AssetId> GameAssetCatalog::idsForTag(std::string_view tag) const {
    return registry_.idsForTag(tag);
}

std::vector<novacore::assets::AssetId> GameAssetCatalog::devSandboxAssetIds() const {
    return idsForTag("dev_sandbox");
}

novacore::assets::AssetStreamingZone GameAssetCatalog::devSandboxStreamingZone() const {
    novacore::assets::AssetStreamingZone zone{};
    zone.name = "dev_sandbox";
    zone.center = {0.0F, 0.0F, 0.0F};
    zone.radiusMeters = 64.0F;
    zone.preloadAssets = devSandboxAssetIds();
    return zone;
}

std::size_t GameAssetCatalog::assetCount() const {
    return registry_.assetCount();
}

} // namespace nemisis::assets
