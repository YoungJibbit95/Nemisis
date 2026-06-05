#pragma once

#include "nemisis/player/PlayerComponents.hpp"
#include "nemisis/player/PlayerInputCommand.hpp"

#include "novacore/net/Loopback.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace nemisis::net {

struct CommandPacket final {
    player::PlayerId playerId = 0;
    std::vector<player::PlayerInputCommand> commands;
};

struct CommandAck final {
    player::PlayerId playerId = 0;
    std::uint64_t acknowledgedTick = 0;
    std::uint64_t serverTick = 0;
    bool accepted = true;
};

[[nodiscard]] std::vector<std::uint8_t> serializeCommandPacket(const CommandPacket& packet);
[[nodiscard]] std::optional<CommandPacket> deserializeCommandPacket(const std::vector<std::uint8_t>& payload);

[[nodiscard]] std::vector<std::uint8_t> serializeCommandAck(const CommandAck& ack);
[[nodiscard]] std::optional<CommandAck> deserializeCommandAck(const std::vector<std::uint8_t>& payload);

[[nodiscard]] novacore::net::Packet makeCommandPacket(
    novacore::net::PacketSequence sequence,
    const CommandPacket& packet);

[[nodiscard]] novacore::net::Packet makeCommandAckPacket(
    novacore::net::PacketSequence sequence,
    const CommandAck& ack);

} // namespace nemisis::net
