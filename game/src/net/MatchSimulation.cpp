#include "MatchSimulation.hpp"
#include <algorithm>

namespace nemisis::net {
void advanceMatch(MatchArena& arena, MatchFrame& frame, std::span<const MatchInput> inputs) {
    if (frame.phase != MatchPhase::Playing || inputs.size() != frame.rules.teamSize * 2) return;
    ++frame.tick;
    --frame.remaining;
    std::array<std::optional<weapons::ShotTraceResult>, maxMatchPlayers> shots{};
    for (unsigned i = 0; i < inputs.size(); ++i) {
        auto& actor = frame.players[i];
        if (actor.respawn && --actor.respawn == 0) {
            arena.spawn(actor, i);
            actor.spawnTick = frame.tick;
        }
        if (inputs[i].command.tick)
            actor.acknowledged = inputs[i].command.tick;
        shots[i] = arena.simulate(actor, inputs[i].command);
    }
    arena.record(frame);
    for (unsigned i = 0; i < inputs.size(); ++i) {
        if (!shots[i])
            continue;
        arena.resolveShot(frame, i, *shots[i], inputs[i].observedTick);
    }
    if (!frame.remaining || std::ranges::max(frame.teamScores) >= frame.rules.scoreLimit)
        frame.phase = MatchPhase::Finished;
}
} // namespace nemisis::net
