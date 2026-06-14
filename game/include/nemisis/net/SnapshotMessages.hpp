#pragma once

#include "nemisis/net/PredictionHistory.hpp"
#include "nemisis/player/PlayerComponents.hpp"

#include "novacore/net/Loopback.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace nemisis::net {

struct PlayerSnapshotState final {
    player::PlayerId playerId = 0;
    std::uint64_t serverTick = 0;
    std::uint64_t acknowledgedCommandTick = 0;
    AuthoritativePlayerState state{};
};

struct SnapshotPacket final {
    std::uint64_t serverTick = 0;
    std::vector<PlayerSnapshotState> players;
};

[[nodiscard]] PlayerSnapshotState makePlayerSnapshotState(
    player::PlayerId playerId,
    std::uint64_t serverTick,
    std::uint64_t acknowledgedCommandTick,
    const movement::PlayerMovementState* movement,
    const player::PlayerViewComponent* view,
    const weapons::WeaponRuntimeState* weapon);

[[nodiscard]] AuthoritativePlayerState authoritativeStateFromSnapshot(const PlayerSnapshotState& snapshot);

[[nodiscard]] std::vector<std::uint8_t> serializeSnapshotPacket(const SnapshotPacket& packet);
[[nodiscard]] std::optional<SnapshotPacket> deserializeSnapshotPacket(const std::vector<std::uint8_t>& payload);

[[nodiscard]] novacore::net::Packet makeSnapshotPacket(
    novacore::net::PacketSequence sequence,
    const SnapshotPacket& packet);

} // namespace nemisis::net
