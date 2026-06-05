#pragma once

#include "novacore/assets/AssetManifest.hpp"
#include "novacore/assets/AssetRegistry.hpp"
#include "novacore/assets/AssetStreamer.hpp"
#include "novacore/assets/AssetTypes.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace nemisis::assets {

struct GameAssetCatalogLoadResult final {
    std::vector<std::string> errors;

    [[nodiscard]] bool ok() const {
        return errors.empty();
    }
};

class GameAssetCatalog final {
public:
    [[nodiscard]] GameAssetCatalogLoadResult loadFromJson(const std::filesystem::path& path);
    void clear();

    [[nodiscard]] const novacore::assets::AssetRecord* find(std::string_view id) const;
    [[nodiscard]] bool contains(std::string_view id) const;
    [[nodiscard]] std::vector<novacore::assets::AssetId> idsForTag(std::string_view tag) const;
    [[nodiscard]] std::vector<novacore::assets::AssetId> devSandboxAssetIds() const;
    [[nodiscard]] novacore::assets::AssetStreamingZone devSandboxStreamingZone() const;
    [[nodiscard]] std::size_t assetCount() const;

private:
    novacore::assets::AssetManifest manifest_;
    novacore::assets::AssetRegistry registry_;
};

} // namespace nemisis::assets
