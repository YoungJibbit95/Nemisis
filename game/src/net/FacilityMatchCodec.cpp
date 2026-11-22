#include "FacilityMatchCodec.hpp"
#include "nemisis/net/CommandMessages.hpp"
#include "nemisis/net/SnapshotMessages.hpp"
#include "novacore/net/BitStream.hpp"
#include <algorithm>
#include <cmath>

namespace nemisis::net {
namespace {
// SnapshotMessages carries transforms, view and weapon. Replay additionally
// needs the complete traversal state at the acknowledgement, including pending
// edges.
template <class Visit, class State> bool visitTraversal(Visit &v, State &s) {
    return v(s.mantleStartPosition) && v(s.mantleTargetPosition) && v(s.mantleNormal) &&
           v(s.groundJumpAvailable) && v(s.dashCooldownRemaining) && v(s.dashTimeRemaining) &&
           v(s.slideCooldownRemaining) && v(s.slideTimeRemaining) && v(s.slideBufferRemaining) &&
           v(s.wallRunContactGraceRemaining) && v(s.wallRunDetachCooldownRemaining) &&
           v(s.doubleJumpBufferRemaining) && v(s.groundedTimeSeconds) && v(s.airborneTimeSeconds) &&
           v(s.crouchFraction) && v(s.landingRecoveryRemaining) && v(s.lastLandingSpeed) && v(s.crouched) &&
           v(s.landedThisTick) && v(s.slideHeldConsumed) && v(s.holoStep.position) &&
           v(s.holoStep.velocity) && v(s.holoStep.ageSeconds) && v(s.holoStep.active) &&
           v(s.tech.wallRunArmTriggerPressed) && v(s.tech.gravityInvertersActive) &&
           v(s.tech.doubleJumpPlatformThrown) && v(s.tech.energyPlatformInFlight) &&
           v(s.tech.wallJumpDetachTriggered) && v(s.tech.mantleReachTriggered) &&
           v(s.tech.mantleClimbTriggered) && v(s.tech.wallRunArmTriggerSeconds) &&
           v(s.tech.gravityInverterGlowSeconds) && v(s.tech.energyPlatformSeconds) &&
           v(s.tech.wallJumpDetachSeconds) && v(s.tech.mantleReachSeconds) && v(s.tech.mantleClimbSeconds) &&
           v(s.tech.energyPlatformCenter) && v(s.tech.energyPlatformPreviousCenter) &&
           v(s.tech.energyPlatformNormal) && v(s.tech.wallRunNormal) && v(s.tech.mantleTargetPosition) &&
           v(s.tech.mantleNormal);
}
struct Write {
    novacore::net::PacketWriter &writer;
    bool operator()(float f) {
        writer.writeFloat(f);
        return true;
    }
    bool operator()(bool b) {
        writer.writeU8(b);
        return true;
    }
    bool operator()(novacore::math::Vec3 v) { return (*this)(v.x) && (*this)(v.y) && (*this)(v.z); }
};
struct Read {
    novacore::net::PacketReader &reader;
    bool operator()(float &f) { return reader.readFloat(f) && std::isfinite(f); }
    bool operator()(bool &b) {
        std::uint8_t raw;
        if (!reader.readU8(raw) || raw > 1)
            return false;
        b = raw != 0;
        return true;
    }
    bool operator()(novacore::math::Vec3 &v) { return (*this)(v.x) && (*this)(v.y) && (*this)(v.z); }
};
} // namespace

void sanitizeMatchCommand(player::PlayerInputCommand &command) {
    const auto bounded = [](float value, float limit) {
        return std::isfinite(value) ? std::clamp(value, -limit, limit) : 0.0F;
    };
    command.move = {bounded(command.move.x, 1), bounded(command.move.y, 1)};
    command.look = {bounded(command.look.x, 180), bounded(command.look.y, 180)};
}
namespace {
constexpr std::uint32_t protocol = 0x4d415404;
// A player can advance at most one command and register one hit per server tick.
// Even the longest supported round fits in three base-128 groups.
void writeTickCounter(novacore::net::PacketWriter& writer, std::uint32_t value) {
    while (value >= 128) {
        writer.writeU8(static_cast<std::uint8_t>(value | 0x80));
        value >>= 7;
    }
    writer.writeU8(static_cast<std::uint8_t>(value));
}
bool readTickCounter(novacore::net::PacketReader& reader, std::uint32_t& value) {
    value = 0;
    for (unsigned shift = 0; shift <= 14; shift += 7) {
        std::uint8_t group;
        if (!reader.readU8(group)) return false;
        value |= static_cast<std::uint32_t>(group & 0x7f) << shift;
        if (group < 128) return (shift == 0 || group != 0) && value <= 216000;
    }
    return false;
}
std::uint8_t velocityByte(float velocity) {
    return static_cast<std::uint8_t>(128 + std::lround(std::clamp(velocity * 4, -128.0F, 127.0F)));
}
void writeRemotePose(novacore::net::PacketWriter& writer, const Duelist& p) {
    const auto v = p.movement.velocity;
    writer.writeU8(velocityByte(v.x));
    writer.writeU8(velocityByte(v.y));
    writer.writeU8(velocityByte(v.z));
    writer.writeU8(static_cast<std::uint8_t>(90 + std::lround(std::clamp(p.view.pitchDegrees, -90.0F, 90.0F))));
    writer.writeU8(static_cast<std::uint8_t>(p.movement.mode) | (p.movement.crouched ? 8 : 0) |
        (p.weapon.adsAlpha > 0.5F ? 16 : 0) | (p.movement.hasWallRunContact ? 32 : 0));
    writeTickCounter(writer, p.weapon.shotIndex);
}
bool readRemotePose(novacore::net::PacketReader& reader, Duelist& p) {
    std::uint8_t x, y, z, pitch, flags;
    if (!reader.readU8(x) || !reader.readU8(y) || !reader.readU8(z) || !reader.readU8(pitch) ||
        pitch > 180 || !reader.readU8(flags) || flags >= 64 ||
        (flags & 7) > static_cast<unsigned>(movement::MovementMode::Mantling) ||
        !readTickCounter(reader, p.weapon.shotIndex)) return false;
    p.movement.velocity = {(x - 128) * 0.25F, (y - 128) * 0.25F, (z - 128) * 0.25F};
    p.view.pitchDegrees = static_cast<float>(pitch) - 90.0F;
    p.movement.mode = static_cast<movement::MovementMode>(flags & 7);
    p.movement.crouched = (flags & 8) != 0;
    p.weapon.adsAlpha = (flags & 16) ? 1.0F : 0.0F;
    p.movement.hasWallRunContact = (flags & 32) != 0;
    return true;
}
constexpr std::uint8_t joinType = 1, challengeType = 2, acceptType = 3, commandType = 4, snapshotType = 5;
void header(novacore::net::PacketWriter &w, std::uint8_t type) {
    w.writeU32(protocol);
    w.writeU8(type);
}
bool header(novacore::net::PacketReader &r, std::uint8_t type) {
    std::uint32_t v;
    std::uint8_t t;
    return r.readU32(v) && v == protocol && r.readU8(t) && t == type;
}
std::vector<std::uint8_t> tokenPacket(std::uint8_t type, std::uint64_t token) {
    novacore::net::PacketWriter w;
    header(w, type);
    w.writeU64(token);
    return w.finish();
}
std::optional<std::uint64_t> readToken(const std::vector<std::uint8_t> &b, std::uint8_t type) {
    novacore::net::PacketReader r(b);
    std::uint64_t token;
    if (!header(r, type) || !r.readU64(token) || !token || !r.consumed())
        return {};
    return token;
}
} // namespace
std::vector<std::uint8_t> encodeMatchJoin() {
    novacore::net::PacketWriter w;
    header(w, joinType);
    w.writeU64(0);
    return w.finish();
}
bool isMatchJoin(const std::vector<std::uint8_t> &b) {
    novacore::net::PacketReader r(b);
    std::uint64_t pad;
    return header(r, joinType) && r.readU64(pad) && !pad && r.consumed();
}
std::vector<std::uint8_t> encodeMatchChallenge(std::uint64_t token) {
    return tokenPacket(challengeType, token);
}
std::optional<std::uint64_t> decodeMatchChallenge(const std::vector<std::uint8_t> &b) {
    return readToken(b, challengeType);
}
std::vector<std::uint8_t> encodeMatchAccept(std::uint64_t token) { return tokenPacket(acceptType, token); }
std::optional<std::uint64_t> decodeMatchAccept(const std::vector<std::uint8_t> &b) {
    return readToken(b, acceptType);
}
std::vector<std::uint8_t> encodeMatchCommands(unsigned slot, std::uint64_t token,
                                              const std::vector<MatchInput> &inputs) {
    if (slot >= maxMatchPlayers || inputs.empty() || inputs.size() > 8)
        return {};
    CommandPacket commands{};
    commands.playerId = slot + 1;
    for (const auto &i : inputs)
        commands.commands.push_back(i.command);
    auto payload = serializeCommandPacket(commands);
    novacore::net::PacketWriter w;
    header(w, commandType);
    w.writeU8(slot);
    w.writeU64(token);
    w.writeU8(static_cast<std::uint8_t>(inputs.size()));
    for (const auto &i : inputs)
        w.writeU32(i.observedTick);
    w.writeBytes(payload);
    return w.finish();
}
std::optional<MatchCommandMessage> decodeMatchCommands(const std::vector<std::uint8_t> &b) {
    if (b.size() > 1200)
        return {};
    novacore::net::PacketReader r(b);
    std::uint8_t slot, count;
    MatchCommandMessage out;
    if (!header(r, commandType) || !r.readU8(slot) || slot >= maxMatchPlayers || !r.readU64(out.token) ||
        !out.token || !r.readU8(count) || !count || count > 8)
        return {};
    out.slot = slot;
    out.inputs.resize(count);
    for (auto &i : out.inputs)
        if (!r.readU32(i.observedTick))
            return {};
    auto payload = r.readByteSpan(r.remaining());
    if (!payload)
        return {};
    auto commands = deserializeCommandPacket({payload->begin(), payload->end()});
    if (!commands || commands->playerId != slot + 1 || commands->commands.size() != count)
        return {};
    for (unsigned i = 0; i < count; ++i) {
        out.inputs[i].command = commands->commands[i];
        if (i && out.inputs[i - 1].command.tick >= out.inputs[i].command.tick)
            return {};
    }
    return out;
}
std::vector<std::uint8_t> encodeMatch(const MatchFrame &frame, unsigned slot, std::uint64_t token) {
    const unsigned count = 2 * frame.rules.teamSize;
    if (!count || count > maxMatchPlayers || slot >= count)
        return {};
    novacore::net::PacketWriter w;
    header(w, snapshotType);
    w.writeU64(token);
    w.writeU8(slot);
    w.writeU8(static_cast<std::uint8_t>(frame.phase));
    w.writeU8(frame.interrupted ? 1 : 0);
    w.writeU32(frame.tick);
    w.writeU32(frame.remaining);
    w.writeU8(static_cast<std::uint8_t>(frame.rules.map));
    w.writeU8(static_cast<std::uint8_t>(frame.rules.mode));
    w.writeU8(static_cast<std::uint8_t>(frame.rules.teamSize));
    w.writeU32(frame.rules.scoreLimit);
    w.writeU32(frame.rules.durationTicks);
    w.writeU32(frame.teamScores[0]);
    w.writeU32(frame.teamScores[1]);
    const auto &local = frame.players[slot];
    SnapshotPacket packet{frame.tick,
                          {makePlayerSnapshotState(slot + 1, frame.tick, local.acknowledged, &local.movement,
                                                   &local.view, &local.weapon)}};
    auto bytes = serializeSnapshotPacket(packet);
    w.writeU16(static_cast<std::uint16_t>(bytes.size()));
    w.writeBytes(bytes);
    Write field{w};
    visitTraversal(field, local.movement);
    writeWeaponRuntime(w, local.holstered);
    for (unsigned i = 0; i < count; ++i) {
        const auto &p = frame.players[i];
        w.writeFloat(p.health.health);
        w.writeU16(static_cast<std::uint16_t>(p.score));
        w.writeU8(static_cast<std::uint8_t>(p.respawn));
        writeTickCounter(w, p.hits);
        writeTickCounter(w, p.spawnTick);
        if (i != slot) {
            field(p.movement.position);
            w.writeFloat(p.movement.crouchFraction);
            w.writeFloat(p.view.yawDegrees);
            writeTickCounter(w, static_cast<std::uint32_t>(p.acknowledged));
            writeRemotePose(w, p);
            w.writeU8(p.weapon.weaponId == "sidearm_01" ? 1 : 0);
            w.writeU16(p.weapon.ammoInMagazine);
            w.writeU16(p.holstered.ammoInMagazine);
        }
    }
    return w.finish();
}
std::optional<MatchMessage> decodeMatch(const std::vector<std::uint8_t> &b) {
    if (b.size() > 1200)
        return {};
    novacore::net::PacketReader r(b);
    MatchMessage out{};
    auto &f = out.frame;
    std::uint8_t slot, phase, interrupted, map, mode, team;
    std::uint16_t size;
    if (!header(r, snapshotType) || !r.readU64(out.token) || !out.token || !r.readU8(slot) ||
        !r.readU8(phase) || phase > 2 || !r.readU8(interrupted) || interrupted > 1 ||
        (interrupted && phase != static_cast<unsigned>(MatchPhase::Finished)) ||
        !r.readU32(f.tick) || !r.readU32(f.remaining) || !r.readU8(map) ||
        map > 1 || !r.readU8(mode) || mode > 1 || !r.readU8(team) || !team || 2 * team > maxMatchPlayers ||
        slot >= 2 * team || !r.readU32(f.rules.scoreLimit) || !r.readU32(f.rules.durationTicks) ||
        !r.readU32(f.teamScores[0]) || !r.readU32(f.teamScores[1]) || !r.readU16(size) ||
        size > r.remaining())
        return {};
    out.slot = slot;
    f.phase = static_cast<MatchPhase>(phase);
    f.interrupted = interrupted != 0;
    f.rules.map = static_cast<MatchMap>(map);
    f.rules.mode = static_cast<MatchMode>(mode);
    f.rules.teamSize = team;
    if (!validMatchRules(f.rules))
        return {};
    auto payload = r.readByteSpan(size);
    auto packet = payload ? deserializeSnapshotPacket({payload->begin(), payload->end()}) : std::nullopt;
    if (!packet || packet->serverTick != f.tick || packet->players.size() != 1 ||
        packet->players[0].playerId != slot + 1)
        return {};
    auto &local = f.players[slot];
    const auto &s = packet->players[0];
    if (!s.state.hasMovement || !s.state.hasView || !s.state.hasWeapon)
        return {};
    local.movement = s.state.movement;
    local.view = s.state.view;
    local.weapon = s.state.weapon;
    local.acknowledged = s.acknowledgedCommandTick;
    Read field{r};
    if (!visitTraversal(field, local.movement) || !readWeaponRuntime(r, local.holstered))
        return {};
    for (unsigned i = 0; i < 2 * team; ++i) {
        auto &p = f.players[i];
        std::uint16_t score;
        std::uint8_t respawn;
        if (!r.readFloat(p.health.health) || !r.readU16(score) || !r.readU8(respawn) || respawn > 90 ||
            !readTickCounter(r, p.hits) || !readTickCounter(r, p.spawnTick))
            return {};
        p.score = score;
        p.respawn = respawn;
        if (!std::isfinite(p.health.health) || p.health.health < 0 || p.health.health > p.health.maxHealth)
            return {};
        p.health.eliminated = p.health.health <= 0;
        if (i != slot) {
            std::uint8_t weapon;
            std::uint32_t ack;
            if (!field(p.movement.position) || !r.readFloat(p.movement.crouchFraction) ||
                !r.readFloat(p.view.yawDegrees) || !readTickCounter(r, ack) || !readRemotePose(r, p) || !r.readU8(weapon) ||
                weapon > 1 || !r.readU16(p.weapon.ammoInMagazine) || !r.readU16(p.holstered.ammoInMagazine))
                return {};
            p.acknowledged = ack;
            p.weapon.weaponId = weapon ? "sidearm_01" : "ar_01";
            p.holstered.weaponId = weapon ? "ar_01" : "sidearm_01";
        }
    }
    for (unsigned i = 0; i < 2 * team; ++i) {
        const auto &p = f.players[i];
        if (!std::isfinite(p.movement.position.x) || !std::isfinite(p.movement.position.y) ||
            !std::isfinite(p.movement.position.z) || !std::isfinite(p.movement.velocity.x) ||
            !std::isfinite(p.movement.velocity.y) || !std::isfinite(p.movement.velocity.z) ||
            !std::isfinite(p.movement.crouchFraction) || !std::isfinite(p.view.pitchDegrees) ||
            !std::isfinite(p.view.yawDegrees))
            return {};
    }
    if (!r.consumed())
        return {};
    return out;
}
} // namespace nemisis::net
