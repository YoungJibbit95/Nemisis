#include "nemisis/player/PlayerHealth.hpp"

#include <algorithm>

namespace nemisis::player {

namespace {

[[nodiscard]] float multiplierForZone(const DamageRequest& request) {
    switch (request.zone) {
    case DamageZone::Head:
        return request.headMultiplier;
    case DamageZone::Legs:
        return request.legMultiplier;
    case DamageZone::Body:
        return 1.0F;
    }
    return 1.0F;
}

} // namespace

void resetHealth(PlayerHealthComponent& health) {
    health.maxHealth = std::max(1.0F, health.maxHealth);
    health.health = health.maxHealth;
    health.eliminated = false;
}

bool isAlive(const PlayerHealthComponent& health) {
    return !health.eliminated && health.health > 0.0F;
}

DamageResult applyDamage(PlayerHealthComponent& health, const DamageRequest& request) {
    DamageResult result{};
    result.healthRemaining = health.health;
    result.critical = request.zone == DamageZone::Head;

    if (!isAlive(health)) {
        result.eliminated = health.eliminated;
        return result;
    }

    const float scaledDamage = std::max(0.0F, request.damage * multiplierForZone(request));
    result.damageApplied = std::min(health.health, scaledDamage);
    health.health = std::max(0.0F, health.health - result.damageApplied);
    health.eliminated = health.health <= 0.0F;

    result.healthRemaining = health.health;
    result.eliminated = health.eliminated;
    return result;
}

} // namespace nemisis::player
