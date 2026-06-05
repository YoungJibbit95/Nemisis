#pragma once

#include "nemisis/net/CommandMessages.hpp"
#include "nemisis/player/PlayerCommandQueue.hpp"

#include "novacore/net/Loopback.hpp"

#include <cstddef>
#include <cstdint>

namespace nemisis::net {

struct LoopbackBridgeStats final {
    std::uint64_t sentCommandPackets = 0;
    std::uint64_t receivedCommandPackets = 0;
    std::uint64_t sentAckPackets = 0;
    std::uint64_t receivedAckPackets = 0;
    std::uint64_t lastSentTick = 0;
    std::uint64_t lastAcknowledgedTick = 0;
};

class LoopbackCommandBridge final {
public:
    void sendPendingCommands(player::PlayerId playerId, const player::PlayerCommandQueue& queue);
    void processServer(std::uint64_t serverTick);
    [[nodiscard]] std::size_t processClientAcks(player::PlayerCommandQueue& queue);

    [[nodiscard]] const LoopbackBridgeStats& stats() const;

private:
    novacore::net::LoopbackChannel channel_;
    novacore::net::PacketSequence nextClientSequence_{1};
    novacore::net::PacketSequence nextServerSequence_{1};
    LoopbackBridgeStats stats_{};
};

} // namespace nemisis::net
