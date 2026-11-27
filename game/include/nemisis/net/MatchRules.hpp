#pragma once

#include "nemisis/dev/GreyboxCollision.hpp"
#include "nemisis/movement/MovementSystem.hpp"
#include "nemisis/weapons/WeaponShot.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace nemisis::net {

enum class MatchMap : std::uint8_t { OuterFacility, DevRange };
enum class MatchMode : std::uint8_t { Duel, TeamDeathmatch };
inline constexpr unsigned maxMatchPlayers = 12;
inline constexpr std::uint32_t maxRewindTicks = 12;

struct MatchRules {
    MatchMap map = MatchMap::OuterFacility;
    MatchMode mode = MatchMode::Duel;
    unsigned teamSize = 1;
    std::uint32_t scoreLimit = 10;
    std::uint32_t durationTicks = 3 * 60 * 60;
};

[[nodiscard]] bool validMatchRules(const MatchRules& rules);

struct Duelist;
struct MatchFrame;

class MatchArena final {
public:
    explicit MatchArena(MatchMap map);
    void spawn(Duelist& actor, unsigned slot) const;
    std::optional<weapons::ShotTraceResult> simulate(Duelist& actor, const player::PlayerInputCommand& input) const;
    void record(const MatchFrame& frame);
    void resolveShot(MatchFrame& frame, unsigned shooter, const weapons::ShotTraceResult& shot,
                     std::uint32_t viewedTick) const;

private:
    struct HitPose {
        novacore::math::Vec3 feet{};
        float yawDegrees = 0;
        float heightScale = 1;
        std::uint32_t spawnTick = 0;
        bool alive = false;
    };
    struct HistoryFrame {
        std::uint32_t tick = 0;
        std::array<HitPose, maxMatchPlayers> poses{};
        bool recorded = false;
    };

    MatchMap map_;
    dev::GreyboxWorld world_;
    novacore::physics::PhysicsWorld collision_;
    movement::MovementSystem motor_;
    weapons::WeaponDefinition rifle_;
    weapons::WeaponDefinition sidearm_;
    std::array<HistoryFrame, maxRewindTicks + 1> history_{};
};

} // namespace nemisis::net
