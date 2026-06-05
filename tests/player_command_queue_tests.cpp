#include "nemisis/player/PlayerCommandQueue.hpp"

#include <cstdint>
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
    command.move.x = static_cast<float>(tick);
    return command;
}

void testQueueKeepsCommandsInTickOrder() {
    nemisis::player::PlayerCommandQueue queue{4};

    expect(queue.push(commandAt(10)), "first command pushes");
    expect(queue.push(commandAt(11)), "later command pushes");

    const auto pending = queue.pendingCommands();
    expect(pending.size() == 2, "queue stores pending commands");
    expect(pending[0].tick == 10, "first pending command keeps order");
    expect(pending[1].tick == 11, "second pending command keeps order");
}

void testDuplicateTickReplacesLatestCommand() {
    nemisis::player::PlayerCommandQueue queue{4};

    auto first = commandAt(4);
    first.fireHeld = false;
    auto replacement = commandAt(4);
    replacement.fireHeld = true;

    expect(queue.push(first), "first duplicate test command pushes");
    expect(queue.push(replacement), "duplicate tick replaces latest command");

    const auto* latest = queue.latest();
    expect(queue.size() == 1, "duplicate tick does not grow queue");
    expect(latest != nullptr && latest->fireHeld, "duplicate tick keeps newest command data");
}

void testOutOfOrderCommandIsRejected() {
    nemisis::player::PlayerCommandQueue queue{4};

    expect(queue.push(commandAt(20)), "baseline command pushes");
    expect(!queue.push(commandAt(19)), "older command is rejected");
    expect(queue.size() == 1, "rejected command does not grow queue");
}

void testCapacityDropsOldestCommand() {
    nemisis::player::PlayerCommandQueue queue{2};

    expect(queue.push(commandAt(1)), "capacity test command 1 pushes");
    expect(queue.push(commandAt(2)), "capacity test command 2 pushes");
    expect(queue.push(commandAt(3)), "capacity test command 3 pushes");

    const auto pending = queue.pendingCommands();
    expect(pending.size() == 2, "queue stays within capacity");
    expect(pending[0].tick == 2, "oldest command is dropped at capacity");
    expect(pending[1].tick == 3, "newest command remains at capacity");
}

void testAcknowledgementRemovesProcessedCommands() {
    nemisis::player::PlayerCommandQueue queue{4};

    expect(queue.push(commandAt(30)), "ack command 30 pushes");
    expect(queue.push(commandAt(31)), "ack command 31 pushes");
    expect(queue.push(commandAt(32)), "ack command 32 pushes");

    const auto removed = queue.acknowledgeThrough(31);
    const auto pending = queue.pendingCommands();

    expect(removed == 2, "ack removes commands through tick");
    expect(pending.size() == 1, "ack leaves later commands pending");
    expect(pending[0].tick == 32, "ack keeps command after acknowledged tick");
}

} // namespace

int main() {
    testQueueKeepsCommandsInTickOrder();
    testDuplicateTickReplacesLatestCommand();
    testOutOfOrderCommandIsRejected();
    testCapacityDropsOldestCommand();
    testAcknowledgementRemovesProcessedCommands();

    if (failures > 0) {
        std::cerr << failures << " player command queue test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis player command queue tests passed\n";
    return EXIT_SUCCESS;
}
