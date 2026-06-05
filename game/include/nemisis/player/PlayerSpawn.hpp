#pragma once

#include "nemisis/player/PlayerComponents.hpp"
#include "nemisis/weapons/WeaponTypes.hpp"

#include "novacore/ecs/Entity.hpp"
#include "novacore/ecs/World.hpp"
#include "novacore/math/Types.hpp"

#include <string>

namespace nemisis::player {

struct LocalPlayerSpawnDesc final {
    PlayerId playerId = 1;
    std::uint8_t teamIndex = 0;
    std::string displayName = "Local Player";
    std::string activeWeaponId = "ar_01";
    novacore::math::Vec3 position{};
};

[[nodiscard]] novacore::ecs::EntityId spawnLocalPlayer(
    novacore::ecs::World& world,
    const LocalPlayerSpawnDesc& desc,
    const weapons::WeaponDefinition* activeWeapon);

} // namespace nemisis::player
