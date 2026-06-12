#include "nemisis/player/PlayerProfile.hpp"

namespace nemisis::player {

AccountStats prototypeAccountStats() {
    AccountStats stats{};
    stats.accountName = "YoungJibbit95";
    stats.level = 7;
    stats.matchesPlayed = 18;
    stats.wins = 9;
    stats.eliminations = 284;
    stats.deaths = 166;
    stats.damagePerMatch = 1850.0F;
    stats.bestWeapon = WeaponStatLine{
        "ar_01",
        "NMC-300 Compact",
        122,
        2450,
        912,
        36400.0F,
    };
    stats.bestOperator = OperatorStatLine{
        "pilot_vanguard",
        "Vanguard Pilot",
        18,
        9,
    };
    return stats;
}

float killDeathRatio(const AccountStats& stats) {
    if (stats.deaths == 0U) {
        return static_cast<float>(stats.eliminations);
    }
    return static_cast<float>(stats.eliminations) / static_cast<float>(stats.deaths);
}

float winRate(const AccountStats& stats) {
    if (stats.matchesPlayed == 0U) {
        return 0.0F;
    }
    return static_cast<float>(stats.wins) / static_cast<float>(stats.matchesPlayed);
}

float weaponAccuracy(const WeaponStatLine& stats) {
    if (stats.shotsFired == 0U) {
        return 0.0F;
    }
    return static_cast<float>(stats.shotsHit) / static_cast<float>(stats.shotsFired);
}

} // namespace nemisis::player
