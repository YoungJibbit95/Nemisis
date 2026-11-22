#include "nemisis/net/FacilityMatch.hpp"
#include "nemisis/net/MatchRules.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string_view>

namespace {
using namespace nemisis::net;

int failures = 0;
void expect(bool condition, std::string_view description) {
    if (!condition) {
        std::cerr << "[fail] " << description << '\n';
        ++failures;
    }
}

nemisis::weapons::ShotTraceResult shotFrom(float x, float y, float z) {
    nemisis::weapons::ShotTraceResult shot{};
    shot.origin = {x, y, z};
    shot.direction = {0, 0, 1};
    shot.rangeMeters = 60;
    return shot;
}

void testRoster() {
    MatchRules rules;
    expect(validMatchRules(rules), "default duel is valid");
    rules.teamSize = 2;
    expect(!validMatchRules(rules), "duel requires exactly one per team");
    rules.mode = MatchMode::TeamDeathmatch;
    for (const auto map : {MatchMap::DevRange, MatchMap::OuterFacility}) {
        rules.map = map;
        MatchArena arena(map);
        rules.teamSize = 6;
        expect(validMatchRules(rules), "both maps host six per side");
        std::set<std::array<float, 3>> positions;
        for (unsigned i = 0; i < maxMatchPlayers; ++i) {
            Duelist actor;
            arena.spawn(actor, i);
            const auto p = actor.movement.position;
            positions.insert({p.x, p.y, p.z});
            arena.simulate(actor, {});
            expect((actor.movement.position - p).lengthSquared() < 0.01F, "spawn clears map collision");
        }
        expect(positions.size() == maxMatchPlayers, "all spawns distinct");
    }
    rules.teamSize = 7;
    expect(!validMatchRules(rules), "oversized roster rejected");
    rules.teamSize = 1;
    rules.map = static_cast<MatchMap>(42);
    expect(!validMatchRules(rules), "unknown map rejected");
}

void testRewindAndLife() {
    MatchArena arena(MatchMap::OuterFacility);
    MatchFrame frame;
    arena.spawn(frame.players[0], 0);
    auto& target = frame.players[1];
    arena.spawn(target, 1);
    const auto shot = shotFrom(0, 1.12F, -26);
    frame.tick = 100;
    arena.record(frame);
    target.movement.position.x += 2;
    frame.tick += maxRewindTicks;
    arena.record(frame);
    arena.resolveShot(frame, 0, shot, frame.tick);
    expect(frame.players[0].hits == 0, "present pose misses");
    arena.resolveShot(frame, 0, shot, 100);
    expect(frame.players[0].hits == 1 && target.health.health < target.health.maxHealth,
           "moving opponent hit at exact rewind limit");
    arena.resolveShot(frame, 0, shot, 99);
    arena.resolveShot(frame, 0, shot, frame.tick + 1);
    expect(frame.players[0].hits == 1, "too-old and future view resolve at present");
    ++target.spawnTick;
    arena.resolveShot(frame, 0, shot, 100);
    expect(frame.players[0].hits == 1, "old body cannot damage a new life");

    // Same ring index as 100, with a new stamp and an off-ray pose.
    ++frame.tick;
    arena.record(frame);
    arena.resolveShot(frame, 0, shot, 100);
    expect(frame.players[0].hits == 1, "expired ring entry cannot score");
}

void testTeamsAndCover() {
    MatchArena arena(MatchMap::OuterFacility);
    MatchFrame frame;
    frame.rules.mode = MatchMode::TeamDeathmatch;
    frame.rules.teamSize = 2;
    for (unsigned i = 0; i < 4; ++i) arena.spawn(frame.players[i], i);
    auto& teammate = frame.players[2];
    auto& nearestEnemy = frame.players[3];
    teammate.movement.position = {0, 0, -23};
    nearestEnemy.movement.position = {0, 0, -20};
    nearestEnemy.health.health = 1;
    frame.tick = 1;
    arena.record(frame);
    auto shot = shotFrom(0, 1.12F, -26);
    arena.resolveShot(frame, 0, shot, 1);
    expect(frame.players[0].hits == 0 && teammate.health.health == teammate.health.maxHealth,
           "teammate blocks a shot without friendly damage");
    teammate.movement.position.x = 2;
    arena.record(frame);
    arena.resolveShot(frame, 0, shot, 1);
    expect(nearestEnemy.health.eliminated && nearestEnemy.respawn == 90, "closest enemy eliminated");
    expect(frame.players[0].score == 1 && frame.teamScores[0] == 1 && frame.teamScores[1] == 0,
           "kill credited to player and team once");
    arena.resolveShot(frame, 0, shot, 1);
    expect(frame.players[0].score == 1, "dead targets yield no duplicate score");
    // West building front wall spans x=-21..-16 at z=-8.
    frame.players[1].movement.position = {-18, 0, -6};
    ++frame.tick;
    arena.record(frame);
    arena.resolveShot(frame, 0, shotFrom(-18, 1.12F, -11), frame.tick);
    expect(frame.players[0].hits == 1, "static wall before target stops hit");
}
}

int main() {
    testRoster();
    testRewindAndLife();
    testTeamsAndCover();
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
