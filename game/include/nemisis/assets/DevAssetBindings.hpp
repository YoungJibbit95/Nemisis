#pragma once

#include "nemisis/assets/GameAssetCatalog.hpp"

#include "novacore/render/Mesh.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace nemisis::assets {

using RequiredDevAssetIds = std::array<std::string_view, 8>;

struct DevAssetBindingSummary final {
    std::size_t requiredAssetCount = 0;
    std::size_t renderableAssetCount = 0;
    std::size_t metadataAssetCount = 0;
    std::size_t missingAssetCount = 0;
    std::vector<std::string> errors;

    [[nodiscard]] bool ready() const {
        return missingAssetCount == 0 && errors.empty() && renderableAssetCount == requiredAssetCount;
    }
};

[[nodiscard]] const RequiredDevAssetIds& requiredDevSandboxRenderableAssetIds();

class DevAssetBindings final {
public:
    void clear();

    [[nodiscard]] DevAssetBindingSummary bindRequiredAssets(
        const GameAssetCatalog& catalog,
        const std::filesystem::path& runtimeRoot);

    [[nodiscard]] DevAssetBindingSummary bindAssets(
        const GameAssetCatalog& catalog,
        std::span<const std::string_view> assetIds,
        const std::filesystem::path& runtimeRoot);

    [[nodiscard]] const novacore::render::MeshCatalog& meshCatalog() const;
    [[nodiscard]] std::size_t meshCount() const;

private:
    novacore::render::MeshCatalog meshes_;
};

} // namespace nemisis::assets
