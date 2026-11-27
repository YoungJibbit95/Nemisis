#include "nemisis/net/FacilityMatch.hpp"
#include "nemisis/dev/DevTargetRange.hpp"
#include "nemisis/player/PlayerCameraRig.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace {
using namespace nemisis::net;
using Clock = std::chrono::steady_clock;
void require(bool value, const char* reason) {
    if (!value)
        throw std::runtime_error(reason);
}

void play(MatchMap map) {
    constexpr unsigned ticks = 2700;
    FacilityMatchClient session(MatchRules{map, MatchMode::TeamDeathmatch, 2, 1000, ticks});
    require(session.isPrivateMatch() && session.slot() == 0 && session.negotiatedWorld(),
            "local roster and map");
    session.tick(nemisis::player::PlayerInputCommand{});
    require(session.authoritative().phase == MatchPhase::Waiting && session.authoritative().tick == 0,
            "presentation gates admission");
    session.confirmMapReady();
    session.tick(std::nullopt);
    require(session.authoritative().tick == 0, "frame polling cannot advance host time");
    std::array<novacore::math::Vec3, 4> previous{};
    std::array<float, 4> distance{};
    std::array<unsigned, 4> shots{}, lastShot{}, lives{};
    std::array<std::uint32_t, 4> lastSpawn{};
    unsigned humanDeaths = 0;
    for (unsigned slot = 0; slot < 4; ++slot)
        previous[slot] = session.authoritative().players[slot].movement.position;
    std::vector<double> costs;
    costs.reserve(ticks);
    for (unsigned tick = 1; tick <= ticks; ++tick) {
        const auto before = Clock::now();
        session.tick(nemisis::player::PlayerInputCommand{});
        costs.push_back(std::chrono::duration<double, std::micro>(Clock::now() - before).count());
        const auto& frame = session.authoritative();
        for (unsigned slot = 0; slot < 4; ++slot) {
            const auto& actor = frame.players[slot];
            if (actor.spawnTick != lastSpawn[slot]) {
                ++lives[slot];
                lastSpawn[slot] = actor.spawnTick;
                lastShot[slot] = 0;
            } else {
                distance[slot] += novacore::math::length(actor.movement.position - previous[slot]);
            }
            previous[slot] = actor.movement.position;
            if (actor.weapon.shotIndex > lastShot[slot])
                shots[slot] += actor.weapon.shotIndex - lastShot[slot];
            lastShot[slot] = actor.weapon.shotIndex;
        }
        if (frame.players[0].health.eliminated)
            ++humanDeaths;
    }
    const auto& f = session.authoritative();
    std::sort(costs.begin(), costs.end());
    std::cout << (map == MatchMap::OuterFacility ? "outer-facility" : "dev-range") << " host_us_p50=" << costs[ticks / 2]
              << " p99=" << costs[ticks * 99 / 100] << " scores=" << f.teamScores[0] << ':' << f.teamScores[1]
              << '\n';
    for (unsigned s = 0; s < 4; ++s)
        std::cout << " slot=" << s << " travel_m=" << distance[s] << " shots=" << shots[s]
                  << " hits=" << f.players[s].hits << " respawns=" << lives[s] << '\n';
    require(f.phase == MatchPhase::Finished && f.tick == ticks && !f.interrupted,
            "ordinary end-of-round rules");
    require(f.players[0].acknowledged == ticks && session.predicted().spawnTick == f.players[0].spawnTick,
            "host mirrors human state");
    for (unsigned s = 1; s < 4; ++s)
        require(distance[s] > 8 && shots[s] > 0 && f.players[s].hits > 0,
                "bots travel, acquire and hit using ordinary commands");
    require(humanDeaths > 0 && lives[0] > 1 && f.teamScores[1] > 1,
            "bots find the human again after a full respawn");
    session.tick(nemisis::player::PlayerInputCommand{});
    require(f.tick == ticks, "finished match remains frozen");
}
void playerCanScore() {
    using namespace nemisis;
    FacilityMatchClient session(MatchRules{MatchMap::OuterFacility, MatchMode::TeamDeathmatch, 2, 1000, 2700});
    session.confirmMapReady();
    for (unsigned tick = 0; tick < 2700 && !session.authoritative().players[0].score; ++tick) {
        const auto& frame = session.authoritative();
        const auto& human = frame.players[0];
        player::PlayerInputCommand input{};
        const auto origin = player::gameplayEyePosition(human.movement.position, human.movement.mode,
                                                         human.movement.crouchFraction);
        for (unsigned slot : {1U, 3U}) {
            const auto& enemy = frame.players[slot];
            if (enemy.health.eliminated) continue;
            const auto target = player::gameplayEyePosition(enemy.movement.position, enemy.movement.mode,
                                                             enemy.movement.crouchFraction);
            const auto towards = target - novacore::math::Vec3{0, .36F, 0} - origin;
            const float range = novacore::math::length(towards);
            if (range < .01F) continue;
            weapons::ShotTraceResult ray{};
            ray.origin = origin;
            ray.direction = towards * (1.F / range);
            ray.rangeMeters = range;
            if (dev::traceDevRangeShot({}, ray, session.negotiatedWorld()->primitives).distanceMeters + .02F < range)
                continue;
            constexpr float degrees = 180.F / std::numbers::pi_v<float>;
            const auto yaw = std::atan2(towards.x, towards.z) * degrees;
            const auto pitch = std::atan2(towards.y, std::hypot(towards.x, towards.z)) * degrees;
            input.look = {std::remainder(human.view.yawDegrees - yaw, 360.F), pitch - human.view.pitchDegrees};
            input.fireHeld = input.adsHeld = true;
            break;
        }
        input.reloadPressed = !human.weapon.ammoInMagazine;
        session.tick(input);
    }
    const auto& frame = session.authoritative();
    std::cout << "human_command tick=" << frame.tick << " hits=" << frame.players[0].hits
              << " eliminations=" << frame.players[0].score << '\n';
    require(frame.players[0].hits > 0 && frame.players[0].score > 0, "human commands can eliminate a private bot");
}
} // namespace
int main() {
    try {
        play(MatchMap::OuterFacility);
        play(MatchMap::DevRange);
        playerCanScore();
        // Re-enter after both previous sessions and their collision/navigation state were released.
        FacilityMatchClient fresh(MatchRules{MatchMap::OuterFacility, MatchMode::TeamDeathmatch, 2, 1, 60});
        require(fresh.authoritative().tick == 0 && fresh.authoritative().teamScores[1] == 0 &&
                    fresh.authoritative().players[0].spawnTick == 0,
                "new match starts clean");
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
