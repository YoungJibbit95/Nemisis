#pragma once
#include "nemisis/net/FacilityMatch.hpp"
#include <optional>
#include <vector>
namespace nemisis::net {
void sanitizeMatchCommand(player::PlayerInputCommand &command);
struct MatchInput {
    player::PlayerInputCommand command;
    std::uint32_t observedTick = 0;
};
struct MatchCommandMessage {
    unsigned slot = 0;
    std::uint64_t token = 0;
    std::vector<MatchInput> inputs;
};
struct MatchMessage {
    unsigned slot = 0;
    std::uint64_t token = 0;
    MatchFrame frame;
};
std::vector<std::uint8_t> encodeMatchJoin();
bool isMatchJoin(const std::vector<std::uint8_t> &);
std::vector<std::uint8_t> encodeMatchChallenge(std::uint64_t token);
std::optional<std::uint64_t> decodeMatchChallenge(const std::vector<std::uint8_t> &);
std::vector<std::uint8_t> encodeMatchAccept(std::uint64_t token);
std::optional<std::uint64_t> decodeMatchAccept(const std::vector<std::uint8_t> &);
std::vector<std::uint8_t> encodeMatchCommands(unsigned slot, std::uint64_t token,
                                              const std::vector<MatchInput> &inputs);
std::optional<MatchCommandMessage> decodeMatchCommands(const std::vector<std::uint8_t> &);
std::vector<std::uint8_t> encodeMatch(const MatchFrame &frame, unsigned slot, std::uint64_t token);
std::optional<MatchMessage> decodeMatch(const std::vector<std::uint8_t> &bytes);
} // namespace nemisis::net
