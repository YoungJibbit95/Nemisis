#include "nemisis/net/LoopbackCommandBridge.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

int failures = 0;

void expect(bool condition, std::string_view message) {
    if (condition) {
        return;
    }

    ++failures;
    std::cerr << "[fail] " << message << '\n';
}

nemisis::player::PlayerInputCommand commandAt(std::uint64_t tick) {
    nemisis::player::PlayerInputCommand command{};
    command.tick = tick;
    command.move.y = 1.0F;
    return command;
}

void testLoopbackBridgeAcknowledgesPendingCommands() {
    nemisis::player::PlayerCommandQueue queue{8};
    expect(queue.push(commandAt(10)), "command 10 queues");
    expect(queue.push(commandAt(11)), "command 11 queues");

    nemisis::net::LoopbackCommandBridge bridge;
    bridge.sendPendingCommands(5, queue);
    bridge.processServer(99);
    const auto removed = bridge.processClientAcks(queue);

    expect(removed == 2, "ack removes pending commands");
    expect(queue.empty(), "queue is empty after ack");
    expect(bridge.stats().sentCommandPackets == 1, "bridge records sent command packet");
    expect(bridge.stats().receivedCommandPackets == 1, "bridge records received command packet");
    expect(bridge.stats().receivedAckPackets == 1, "bridge records received ack packet");
    expect(bridge.stats().lastAcknowledgedTick == 11, "bridge records last acknowledged tick");
}

} // namespace

int main() {
    testLoopbackBridgeAcknowledgesPendingCommands();

    if (failures > 0) {
        std::cerr << failures << " loopback command bridge test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis loopback command bridge tests passed\n";
    return EXIT_SUCCESS;
}
