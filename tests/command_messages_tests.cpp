#include "nemisis/net/CommandMessages.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, std::string_view message) {
    if (condition) {
        return;
    }

    ++failures;
    std::cerr << "[fail] " << message << '\n';
}

void expectNear(float actual, float expected, float tolerance, std::string_view message) {
    expect(std::abs(actual - expected) <= tolerance, message);
}

nemisis::player::PlayerInputCommand sampleCommand(std::uint64_t tick) {
    nemisis::player::PlayerInputCommand command{};
    command.tick = tick;
    command.move.x = 0.25F;
    command.move.y = 0.75F;
    command.look.x = 1.5F;
    command.look.y = -0.25F;
    command.jumpPressed = true;
    command.fireHeld = true;
    command.adsHeld = true;
    command.device = novacore::platform::InputDeviceKind::Controller;
    return command;
}

void testCommandPacketRoundTrip() {
    nemisis::net::CommandPacket packet{};
    packet.playerId = 7;
    packet.commands.push_back(sampleCommand(100));
    packet.commands.push_back(sampleCommand(101));

    const auto payload = nemisis::net::serializeCommandPacket(packet);
    const auto decoded = nemisis::net::deserializeCommandPacket(payload);

    expect(decoded.has_value(), "command packet decodes");
    expect(decoded.has_value() && decoded->playerId == 7, "command packet keeps player id");
    expect(decoded.has_value() && decoded->commands.size() == 2, "command packet keeps command count");

    const auto& command = decoded->commands[1];
    expect(command.tick == 101, "command packet keeps tick");
    expectNear(command.move.y, 0.75F, 0.001F, "command packet keeps move");
    expectNear(command.look.x, 1.5F, 0.001F, "command packet keeps look");
    expect(command.jumpPressed, "command packet keeps jump");
    expect(command.fireHeld, "command packet keeps fire");
    expect(command.device == novacore::platform::InputDeviceKind::Controller, "command packet keeps device");
}

void testCommandAckRoundTrip() {
    nemisis::net::CommandAck ack{};
    ack.playerId = 3;
    ack.acknowledgedTick = 55;
    ack.serverTick = 60;
    ack.accepted = true;

    const auto payload = nemisis::net::serializeCommandAck(ack);
    const auto decoded = nemisis::net::deserializeCommandAck(payload);

    expect(decoded.has_value(), "ack decodes");
    expect(decoded.has_value() && decoded->playerId == 3, "ack keeps player id");
    expect(decoded.has_value() && decoded->acknowledgedTick == 55, "ack keeps acknowledged tick");
    expect(decoded.has_value() && decoded->serverTick == 60, "ack keeps server tick");
    expect(decoded.has_value() && decoded->accepted, "ack keeps accepted flag");
}

void testInvalidPayloadFails() {
    const std::vector<std::uint8_t> invalid{1, 2, 3};
    expect(!nemisis::net::deserializeCommandPacket(invalid).has_value(), "invalid command packet fails");
    expect(!nemisis::net::deserializeCommandAck(invalid).has_value(), "invalid ack fails");
}

} // namespace

int main() {
    testCommandPacketRoundTrip();
    testCommandAckRoundTrip();
    testInvalidPayloadFails();

    if (failures > 0) {
        std::cerr << failures << " command message test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis command message tests passed\n";
    return EXIT_SUCCESS;
}
