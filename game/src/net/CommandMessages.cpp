#include "nemisis/net/CommandMessages.hpp"

#include "novacore/net/BitStream.hpp"

#include <limits>

namespace nemisis::net {

namespace {

constexpr std::uint32_t kCommandMagic = 0x4E434D44U;
constexpr std::uint32_t kAckMagic = 0x4E41434BU;
constexpr std::uint16_t kProtocolVersion = 1;

void writeCommand(novacore::net::PacketWriter& writer, const player::PlayerInputCommand& command) {
    writer.writeU64(command.tick);
    writer.writeFloat(command.move.x);
    writer.writeFloat(command.move.y);
    writer.writeFloat(command.look.x);
    writer.writeFloat(command.look.y);
    writer.writeU8(command.jumpPressed ? 1U : 0U);
    writer.writeU8(command.doubleJumpPressed ? 1U : 0U);
    writer.writeU8(command.crouchHeld ? 1U : 0U);
    writer.writeU8(command.slidePressed ? 1U : 0U);
    writer.writeU8(command.sprintHeld ? 1U : 0U);
    writer.writeU8(command.tacticalSprintHeld ? 1U : 0U);
    writer.writeU8(command.dashPressed ? 1U : 0U);
    writer.writeU8(command.mantlePressed ? 1U : 0U);
    writer.writeU8(command.fireHeld ? 1U : 0U);
    writer.writeU8(command.adsHeld ? 1U : 0U);
    writer.writeU8(command.reloadPressed ? 1U : 0U);
    writer.writeU8(static_cast<std::uint8_t>(command.device));
}

[[nodiscard]] bool readBool(novacore::net::PacketReader& reader, bool& value) {
    std::uint8_t raw = 0;
    if (!reader.readU8(raw)) {
        return false;
    }
    value = raw != 0;
    return true;
}

[[nodiscard]] bool readCommand(novacore::net::PacketReader& reader, player::PlayerInputCommand& command) {
    std::uint8_t device = 0;
    if (!reader.readU64(command.tick) ||
        !reader.readFloat(command.move.x) ||
        !reader.readFloat(command.move.y) ||
        !reader.readFloat(command.look.x) ||
        !reader.readFloat(command.look.y) ||
        !readBool(reader, command.jumpPressed) ||
        !readBool(reader, command.doubleJumpPressed) ||
        !readBool(reader, command.crouchHeld) ||
        !readBool(reader, command.slidePressed) ||
        !readBool(reader, command.sprintHeld) ||
        !readBool(reader, command.tacticalSprintHeld) ||
        !readBool(reader, command.dashPressed) ||
        !readBool(reader, command.mantlePressed) ||
        !readBool(reader, command.fireHeld) ||
        !readBool(reader, command.adsHeld) ||
        !readBool(reader, command.reloadPressed) ||
        !reader.readU8(device)) {
        return false;
    }

    command.device = static_cast<novacore::platform::InputDeviceKind>(device);
    return true;
}

} // namespace

std::vector<std::uint8_t> serializeCommandPacket(const CommandPacket& packet) {
    novacore::net::PacketWriter writer;
    writer.writeU32(kCommandMagic);
    writer.writeU16(kProtocolVersion);
    writer.writeU32(packet.playerId);
    const auto commandCount = static_cast<std::uint16_t>(
        packet.commands.size() > std::numeric_limits<std::uint16_t>::max()
            ? std::numeric_limits<std::uint16_t>::max()
            : packet.commands.size());
    writer.writeU16(commandCount);
    for (std::uint16_t index = 0; index < commandCount; ++index) {
        writeCommand(writer, packet.commands[index]);
    }
    return writer.finish();
}

std::optional<CommandPacket> deserializeCommandPacket(const std::vector<std::uint8_t>& payload) {
    novacore::net::PacketReader reader(payload);
    std::uint32_t magic = 0;
    std::uint16_t version = 0;
    std::uint32_t playerId = 0;
    std::uint16_t commandCount = 0;

    if (!reader.readU32(magic) || !reader.readU16(version) || magic != kCommandMagic || version != kProtocolVersion ||
        !reader.readU32(playerId) || !reader.readU16(commandCount)) {
        return std::nullopt;
    }

    CommandPacket packet{};
    packet.playerId = playerId;
    packet.commands.reserve(commandCount);
    for (std::uint16_t index = 0; index < commandCount; ++index) {
        player::PlayerInputCommand command{};
        if (!readCommand(reader, command)) {
            return std::nullopt;
        }
        packet.commands.push_back(command);
    }

    if (!reader.consumed()) {
        return std::nullopt;
    }
    return packet;
}

std::vector<std::uint8_t> serializeCommandAck(const CommandAck& ack) {
    novacore::net::PacketWriter writer;
    writer.writeU32(kAckMagic);
    writer.writeU16(kProtocolVersion);
    writer.writeU32(ack.playerId);
    writer.writeU64(ack.acknowledgedTick);
    writer.writeU64(ack.serverTick);
    writer.writeU8(ack.accepted ? 1U : 0U);
    return writer.finish();
}

std::optional<CommandAck> deserializeCommandAck(const std::vector<std::uint8_t>& payload) {
    novacore::net::PacketReader reader(payload);
    std::uint32_t magic = 0;
    std::uint16_t version = 0;
    std::uint8_t accepted = 0;

    CommandAck ack{};
    if (!reader.readU32(magic) || !reader.readU16(version) || magic != kAckMagic || version != kProtocolVersion ||
        !reader.readU32(ack.playerId) ||
        !reader.readU64(ack.acknowledgedTick) ||
        !reader.readU64(ack.serverTick) ||
        !reader.readU8(accepted) ||
        !reader.consumed()) {
        return std::nullopt;
    }

    ack.accepted = accepted != 0;
    return ack;
}

novacore::net::Packet makeCommandPacket(
    novacore::net::PacketSequence sequence,
    const CommandPacket& packet) {
    novacore::net::Packet transport{};
    transport.sequence = sequence;
    transport.payload = serializeCommandPacket(packet);
    return transport;
}

novacore::net::Packet makeCommandAckPacket(
    novacore::net::PacketSequence sequence,
    const CommandAck& ack) {
    novacore::net::Packet transport{};
    transport.sequence = sequence;
    transport.payload = serializeCommandAck(ack);
    return transport;
}

} // namespace nemisis::net
