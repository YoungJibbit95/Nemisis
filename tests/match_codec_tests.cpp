#include "../game/src/net/FacilityMatchCodec.hpp"
#include <iostream>
#include <limits>

int main() {
    using namespace nemisis::net;
    MatchFrame frame{};
    frame.rules.mode = MatchMode::TeamDeathmatch;
    frame.rules.teamSize = 6;
    frame.tick = 42;
    frame.phase = MatchPhase::Playing;
    frame.teamScores = {7, 5};
    for (unsigned i = 0; i < maxMatchPlayers; ++i) {
        auto &p = frame.players[i];
        p.weapon.weaponId = i % 2 ? "sidearm_01" : "ar_01";
        p.holstered.weaponId = i % 2 ? "ar_01" : "sidearm_01";
        p.weapon.ammoInMagazine = static_cast<std::uint16_t>(i + 1);
        p.holstered.ammoInMagazine = static_cast<std::uint16_t>(i + 2);
        p.movement.position.x = static_cast<float>(i);
        p.movement.velocity = {1.5F, -2.0F, 3.25F};
        p.movement.mode = nemisis::movement::MovementMode::Sliding;
        p.view.pitchDegrees = -13;
        p.weapon.shotIndex = 16000;
        p.acknowledged = i + 20;
    }
    const auto bytes = encodeMatch(frame, 3, 123);
    const auto decoded = decodeMatch(bytes);
    if (!decoded || bytes.size() > 1200 || decoded->token != 123 ||
        decoded->frame.teamScores != frame.teamScores)
        return 1;
    for (unsigned i = 0; i < maxMatchPlayers; ++i) {
        const auto &a = frame.players[i];
        const auto &b = decoded->frame.players[i];
        if (a.weapon.weaponId != b.weapon.weaponId || a.weapon.ammoInMagazine != b.weapon.ammoInMagazine ||
            a.holstered.ammoInMagazine != b.holstered.ammoInMagazine || a.acknowledged != b.acknowledged ||
            a.movement.position.x != b.movement.position.x ||
            a.movement.velocity.y != b.movement.velocity.y || a.movement.mode != b.movement.mode ||
            a.view.pitchDegrees != b.view.pitchDegrees || a.weapon.shotIndex != b.weapon.shotIndex)
            return 2;
    }
    // Last tick of a one-hour round exercises every counter's longest legal encoding.
    auto largest = frame;
    for (auto &p : largest.players) {
        p.hits = p.spawnTick = p.weapon.shotIndex = 216000;
        p.acknowledged = 216000;
    }
    const auto maximum = encodeMatch(largest, 0, 456);
    const auto worst = decodeMatch(maximum);
    if (!worst || maximum.size() > 1200 || worst->frame.players[11].hits != 216000 ||
        worst->frame.players[11].acknowledged != 216000)
        return 8;
    auto bad = bytes;
    bad[0] ^= 1;
    if (decodeMatch(bad))
        return 3;
    bad = bytes;
    bad.pop_back();
    if (decodeMatch(bad))
        return 4;
    frame.players[0].health.health = std::numeric_limits<float>::quiet_NaN();
    if (decodeMatch(encodeMatch(frame, 3, 123)))
        return 5;
    frame.players[0].health.health = frame.players[0].health.maxHealth;
    frame.players[3].weapon.reloadTimeRemaining = std::numeric_limits<float>::infinity();
    if (decodeMatch(encodeMatch(frame, 3, 123)))
        return 9;
    frame.players[3].weapon.reloadTimeRemaining = 0.0F;
    frame.phase = MatchPhase::Finished;
    frame.interrupted = true;
    const auto interrupted = decodeMatch(encodeMatch(frame, 3, 123));
    if (!interrupted || !interrupted->frame.interrupted)
        return 10;
    MatchInput input{};
    input.command.tick = 12;
    input.command.fireHeld = true;
    input.observedTick = 8;
    const auto command = decodeMatchCommands(encodeMatchCommands(3, 123, {input}));
    if (!command || command->token != 123 || command->inputs[0].observedTick != 8 ||
        !command->inputs[0].command.fireHeld)
        return 6;
    if (!isMatchJoin(encodeMatchJoin()) || decodeMatchChallenge(encodeMatchAccept(123)) ||
        decodeMatchAccept(encodeMatchAccept(123)) != 123)
        return 7;
    std::cout << "Match codec bytes: " << bytes.size() << '\n';
    return 0;
}
