#pragma once

#include <cstdint>
#include <string>

namespace nemisis::player {

struct WeaponStatLine final {
    std::string weaponId = "ar_01";
    std::string displayName = "Prototype AR";
    std::uint32_t eliminations = 0;
    std::uint32_t shotsFired = 0;
    std::uint32_t shotsHit = 0;
    float damageDealt = 0.0F;
};

struct OperatorStatLine final {
    std::string operatorId = "pilot_vanguard";
    std::string displayName = "Vanguard Pilot";
    std::uint32_t matchesPlayed = 0;
    std::uint32_t wins = 0;
};

struct AccountStats final {
    std::string accountName = "YoungJibbit95";
    std::uint32_t level = 1;
    std::uint32_t matchesPlayed = 0;
    std::uint32_t wins = 0;
    std::uint32_t eliminations = 0;
    std::uint32_t deaths = 0;
    float damagePerMatch = 0.0F;
    WeaponStatLine bestWeapon{};
    OperatorStatLine bestOperator{};
};

[[nodiscard]] AccountStats prototypeAccountStats();
[[nodiscard]] float killDeathRatio(const AccountStats& stats);
[[nodiscard]] float winRate(const AccountStats& stats);
[[nodiscard]] float weaponAccuracy(const WeaponStatLine& stats);

} // namespace nemisis::player
