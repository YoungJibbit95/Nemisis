#pragma once

#include "nemisis/player/PlayerComponents.hpp"

#include <cstdint>

namespace nemisis::player {

enum class DamageZone {
    Body,
    Head,
    Legs
};

struct DamageRequest final {
    float damage = 0.0F;
    DamageZone zone = DamageZone::Body;
    float headMultiplier = 1.0F;
    float legMultiplier = 0.85F;
    PlayerId sourcePlayerId = 0;
    std::uint64_t tick = 0;
};

struct DamageResult final {
    float damageApplied = 0.0F;
    float healthRemaining = 0.0F;
    bool critical = false;
    bool eliminated = false;
};

void resetHealth(PlayerHealthComponent& health);
[[nodiscard]] bool isAlive(const PlayerHealthComponent& health);
[[nodiscard]] DamageResult applyDamage(PlayerHealthComponent& health, const DamageRequest& request);

} // namespace nemisis::player
