#include "nemisis/net/SnapshotMessages.hpp"

#include "novacore/net/BitStream.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

namespace nemisis::net {

namespace {

constexpr std::uint32_t kSnapshotMagic = 0x4E534E50U;
constexpr std::uint16_t kProtocolVersion = 1;
constexpr std::size_t kMaxWeaponIdBytes = 96;

[[nodiscard]] bool readFinite(novacore::net::PacketReader& reader, float& value) {
    return reader.readFloat(value) && std::isfinite(value);
}

void writeVec3(novacore::net::PacketWriter& writer, novacore::math::Vec3 value) {
    writer.writeFloat(value.x);
    writer.writeFloat(value.y);
    writer.writeFloat(value.z);
}

[[nodiscard]] bool readVec3(novacore::net::PacketReader& reader, novacore::math::Vec3& value) {
    return readFinite(reader, value.x) &&
        readFinite(reader, value.y) &&
        readFinite(reader, value.z);
}

void writeString(novacore::net::PacketWriter& writer, const std::string& value) {
    const auto length = static_cast<std::uint16_t>(std::min<std::size_t>(
        value.size(),
        std::numeric_limits<std::uint16_t>::max()));
    writer.writeU16(length);
    writer.writeBytes(std::string_view(value.data(), length));
}

[[nodiscard]] bool readString(novacore::net::PacketReader& reader, std::string& value) {
    std::uint16_t length = 0;
    if (!reader.readU16(length) || length > kMaxWeaponIdBytes) {
        return false;
    }
    const auto bytes = reader.readBytes(length);
    if (!bytes.has_value()) {
        return false;
    }
    value = std::string(*bytes);
    return true;
}

void writeMovement(novacore::net::PacketWriter& writer, const movement::PlayerMovementState& movement) {
    writeVec3(writer, movement.position);
    writeVec3(writer, movement.velocity);
    writeVec3(writer, movement.wallRunNormal);
    writeVec3(writer, movement.wallRunTangent);
    writer.writeU8(static_cast<std::uint8_t>(movement.mode));
    writer.writeU8(movement.hasDoubleJump ? 1U : 0U);
    writer.writeU8(movement.hasWallRunContact ? 1U : 0U);
    writer.writeFloat(movement.lastHorizontalSpeed);
    writer.writeFloat(movement.inputMagnitude);
    writer.writeFloat(movement.coyoteTimeRemaining);
    writer.writeFloat(movement.jumpBufferRemaining);
    writer.writeFloat(movement.wallRunTimeRemaining);
    writer.writeFloat(movement.mantleTimeRemaining);
    writer.writeFloat(movement.mantleProgressSeconds);
}

[[nodiscard]] bool readMovement(
    novacore::net::PacketReader& reader,
    movement::PlayerMovementState& movement) {
    std::uint8_t mode = 0;
    std::uint8_t hasDoubleJump = 0;
    std::uint8_t hasWallRunContact = 0;
    if (!readVec3(reader, movement.position) ||
        !readVec3(reader, movement.velocity) ||
        !readVec3(reader, movement.wallRunNormal) ||
        !readVec3(reader, movement.wallRunTangent) ||
        !reader.readU8(mode) ||
        !reader.readU8(hasDoubleJump) ||
        !reader.readU8(hasWallRunContact) ||
        !readFinite(reader, movement.lastHorizontalSpeed) ||
        !readFinite(reader, movement.inputMagnitude) ||
        !readFinite(reader, movement.coyoteTimeRemaining) ||
        !readFinite(reader, movement.jumpBufferRemaining) ||
        !readFinite(reader, movement.wallRunTimeRemaining) ||
        !readFinite(reader, movement.mantleTimeRemaining) ||
        !readFinite(reader, movement.mantleProgressSeconds)) {
        return false;
    }
    if (mode > static_cast<std::uint8_t>(movement::MovementMode::Mantling)) {
        return false;
    }
    movement.mode = static_cast<movement::MovementMode>(mode);
    movement.hasDoubleJump = hasDoubleJump != 0;
    movement.hasWallRunContact = hasWallRunContact != 0;
    return true;
}

void writeWeapon(novacore::net::PacketWriter& writer, const weapons::WeaponRuntimeState& weapon) {
    writeString(writer, weapon.weaponId);
    writer.writeU16(weapon.ammoInMagazine);
    writer.writeU32(weapon.shotIndex);
    writer.writeU32(weapon.burstShotCount);
    writer.writeFloat(weapon.fireCooldownRemaining);
    writer.writeFloat(weapon.reloadTimeRemaining);
    writer.writeFloat(weapon.adsAlpha);
    writer.writeFloat(weapon.recoilPitchOffsetDegrees);
    writer.writeFloat(weapon.recoilYawOffsetDegrees);
    writer.writeFloat(weapon.timeSinceLastShotSeconds);
    writer.writeFloat(weapon.reloadProgress);
    writer.writeU8(weapon.reloading ? 1U : 0U);
}

[[nodiscard]] bool readWeapon(
    novacore::net::PacketReader& reader,
    weapons::WeaponRuntimeState& weapon) {
    std::uint8_t reloading = 0;
    if (!readString(reader, weapon.weaponId) ||
        !reader.readU16(weapon.ammoInMagazine) ||
        !reader.readU32(weapon.shotIndex) ||
        !reader.readU32(weapon.burstShotCount) ||
        !readFinite(reader, weapon.fireCooldownRemaining) ||
        !readFinite(reader, weapon.reloadTimeRemaining) ||
        !readFinite(reader, weapon.adsAlpha) ||
        !readFinite(reader, weapon.recoilPitchOffsetDegrees) ||
        !readFinite(reader, weapon.recoilYawOffsetDegrees) ||
        !readFinite(reader, weapon.timeSinceLastShotSeconds) ||
        !readFinite(reader, weapon.reloadProgress) ||
        !reader.readU8(reloading)) {
        return false;
    }
    weapon.reloading = reloading != 0;
    return true;
}

void writePlayerSnapshot(novacore::net::PacketWriter& writer, const PlayerSnapshotState& snapshot) {
    writer.writeU32(snapshot.playerId);
    writer.writeU64(snapshot.serverTick);
    writer.writeU64(snapshot.acknowledgedCommandTick);
    writer.writeU64(snapshot.state.tick);
    writer.writeU8(snapshot.state.hasMovement ? 1U : 0U);
    writer.writeU8(snapshot.state.hasView ? 1U : 0U);
    writer.writeU8(snapshot.state.hasWeapon ? 1U : 0U);
    if (snapshot.state.hasMovement) {
        writeMovement(writer, snapshot.state.movement);
    }
    if (snapshot.state.hasView) {
        writer.writeFloat(snapshot.state.view.yawDegrees);
        writer.writeFloat(snapshot.state.view.pitchDegrees);
    }
    if (snapshot.state.hasWeapon) {
        writeWeapon(writer, snapshot.state.weapon);
    }
}

[[nodiscard]] bool readPlayerSnapshot(
    novacore::net::PacketReader& reader,
    PlayerSnapshotState& snapshot) {
    std::uint8_t hasMovement = 0;
    std::uint8_t hasView = 0;
    std::uint8_t hasWeapon = 0;
    if (!reader.readU32(snapshot.playerId) ||
        !reader.readU64(snapshot.serverTick) ||
        !reader.readU64(snapshot.acknowledgedCommandTick) ||
        !reader.readU64(snapshot.state.tick) ||
        !reader.readU8(hasMovement) ||
        !reader.readU8(hasView) ||
        !reader.readU8(hasWeapon)) {
        return false;
    }

    snapshot.state.hasMovement = hasMovement != 0;
    snapshot.state.hasView = hasView != 0;
    snapshot.state.hasWeapon = hasWeapon != 0;
    if (snapshot.state.hasMovement && !readMovement(reader, snapshot.state.movement)) {
        return false;
    }
    if (snapshot.state.hasView &&
        (!readFinite(reader, snapshot.state.view.yawDegrees) ||
            !readFinite(reader, snapshot.state.view.pitchDegrees))) {
        return false;
    }
    if (snapshot.state.hasWeapon && !readWeapon(reader, snapshot.state.weapon)) {
        return false;
    }
    return true;
}

} // namespace

void writeWeaponRuntime(novacore::net::PacketWriter& writer, const weapons::WeaponRuntimeState& state) {
    writeWeapon(writer, state);
}
bool readWeaponRuntime(novacore::net::PacketReader& reader, weapons::WeaponRuntimeState& state) {
    return readWeapon(reader, state);
}

PlayerSnapshotState makePlayerSnapshotState(
    player::PlayerId playerId,
    std::uint64_t serverTick,
    std::uint64_t acknowledgedCommandTick,
    const movement::PlayerMovementState* movement,
    const player::PlayerViewComponent* view,
    const weapons::WeaponRuntimeState* weapon) {
    PlayerSnapshotState result{};
    result.playerId = playerId;
    result.serverTick = serverTick;
    result.acknowledgedCommandTick = acknowledgedCommandTick;
    result.state = makeAuthoritativeState(acknowledgedCommandTick, movement, view, weapon);
    return result;
}

AuthoritativePlayerState authoritativeStateFromSnapshot(const PlayerSnapshotState& snapshot) {
    return snapshot.state;
}

std::vector<std::uint8_t> serializeSnapshotPacket(const SnapshotPacket& packet) {
    novacore::net::PacketWriter writer;
    writer.writeU32(kSnapshotMagic);
    writer.writeU16(kProtocolVersion);
    writer.writeU64(packet.serverTick);
    const auto count = static_cast<std::uint16_t>(std::min<std::size_t>(
        packet.players.size(),
        std::numeric_limits<std::uint16_t>::max()));
    writer.writeU16(count);
    for (std::uint16_t index = 0; index < count; ++index) {
        writePlayerSnapshot(writer, packet.players[index]);
    }
    return writer.finish();
}

std::optional<SnapshotPacket> deserializeSnapshotPacket(const std::vector<std::uint8_t>& payload) {
    novacore::net::PacketReader reader(payload);
    std::uint32_t magic = 0;
    std::uint16_t version = 0;
    std::uint16_t count = 0;

    SnapshotPacket packet{};
    if (!reader.readU32(magic) ||
        !reader.readU16(version) ||
        magic != kSnapshotMagic ||
        version != kProtocolVersion ||
        !reader.readU64(packet.serverTick) ||
        !reader.readU16(count) || count > reader.remaining() / 31) {
        return std::nullopt;
    }

    packet.players.reserve(count);
    for (std::uint16_t index = 0; index < count; ++index) {
        PlayerSnapshotState snapshot{};
        if (!readPlayerSnapshot(reader, snapshot)) {
            return std::nullopt;
        }
        packet.players.push_back(std::move(snapshot));
    }

    if (!reader.consumed()) {
        return std::nullopt;
    }
    return packet;
}

novacore::net::Packet makeSnapshotPacket(
    novacore::net::PacketSequence sequence,
    const SnapshotPacket& packet) {
    novacore::net::Packet transport{};
    transport.sequence = sequence;
    transport.payload = serializeSnapshotPacket(packet);
    return transport;
}

} // namespace nemisis::net
