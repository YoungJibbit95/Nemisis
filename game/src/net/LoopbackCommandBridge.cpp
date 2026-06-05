#include "nemisis/net/LoopbackCommandBridge.hpp"

namespace nemisis::net {

void LoopbackCommandBridge::sendPendingCommands(player::PlayerId playerId, const player::PlayerCommandQueue& queue) {
    const auto pending = queue.pendingCommands();
    if (pending.empty()) {
        return;
    }

    CommandPacket packet{};
    packet.playerId = playerId;
    packet.commands = pending;

    stats_.lastSentTick = pending.back().tick;
    ++stats_.sentCommandPackets;
    channel_.sendToServer(makeCommandPacket(nextClientSequence_, packet));
    nextClientSequence_ = nextClientSequence_.next();
}

void LoopbackCommandBridge::processServer(std::uint64_t serverTick) {
    novacore::net::Packet transport{};
    while (channel_.tryReceiveForServer(transport)) {
        const auto packet = deserializeCommandPacket(transport.payload);
        if (!packet.has_value() || packet->commands.empty()) {
            continue;
        }

        ++stats_.receivedCommandPackets;

        CommandAck ack{};
        ack.playerId = packet->playerId;
        ack.acknowledgedTick = packet->commands.back().tick;
        ack.serverTick = serverTick;
        ack.accepted = true;

        ++stats_.sentAckPackets;
        channel_.sendToClient(makeCommandAckPacket(nextServerSequence_, ack));
        nextServerSequence_ = nextServerSequence_.next();
    }
}

std::size_t LoopbackCommandBridge::processClientAcks(player::PlayerCommandQueue& queue) {
    std::size_t removed = 0;
    novacore::net::Packet transport{};
    while (channel_.tryReceiveForClient(transport)) {
        const auto ack = deserializeCommandAck(transport.payload);
        if (!ack.has_value() || !ack->accepted) {
            continue;
        }

        ++stats_.receivedAckPackets;
        stats_.lastAcknowledgedTick = ack->acknowledgedTick;
        removed += queue.acknowledgeThrough(ack->acknowledgedTick);
    }
    return removed;
}

const LoopbackBridgeStats& LoopbackCommandBridge::stats() const {
    return stats_;
}

} // namespace nemisis::net
