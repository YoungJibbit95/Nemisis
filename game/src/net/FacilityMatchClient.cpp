#include "FacilityMatchCodec.hpp"
#include "nemisis/net/FacilityMatch.hpp"
#include "nemisis/net/MatchRules.hpp"
#include "novacore/net/LocalDatagram.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <deque>
#include <stdexcept>
namespace nemisis::net {
namespace {
using Clock = std::chrono::steady_clock;
using Command = player::PlayerInputCommand;

} // namespace
struct FacilityMatchClient::Impl {
    explicit Impl(novacore::net::DatagramEndpoint endpoint)
        : remote(endpoint), socket(novacore::net::DatagramEndpoint::any(0)) {}
    novacore::net::DatagramEndpoint remote;
    novacore::net::DatagramSocket socket;
    std::unique_ptr<MatchArena> arena;
    unsigned slot = 0;
    MatchFrame frame{};
    Duelist predicted{};
    std::deque<MatchInput> pending;
    std::uint64_t nextInput = 0, token = 0;
    bool received = false;
    Clock::time_point lastSeen = Clock::now();
    Clock::time_point lastRetry{};
    Clock::time_point challengeTime{};
};
FacilityMatchClient::FacilityMatchClient(std::uint16_t port)
    : FacilityMatchClient(*novacore::net::DatagramEndpoint::parse("127.0.0.1", port)) {}
FacilityMatchClient::FacilityMatchClient(novacore::net::DatagramEndpoint remote)
    : impl_(std::make_unique<Impl>(remote)) {}
FacilityMatchClient::~FacilityMatchClient() = default;
unsigned FacilityMatchClient::slot() const { return impl_->slot; }
const MatchFrame &FacilityMatchClient::authoritative() const { return impl_->frame; }
const Duelist &FacilityMatchClient::predicted() const { return impl_->predicted; }
void FacilityMatchClient::tick(std::optional<Command> command) {
    auto &c = *impl_;
    const bool wasPlaying = c.frame.phase == MatchPhase::Playing;
    const auto observedTick = c.frame.tick;
    if (c.frame.phase == MatchPhase::Finished)
        return;
    std::optional<MatchMessage> newest;
    for (unsigned i = 0; i < 64; ++i) {
        auto packet = c.socket.receive();
        if (!packet)
            break;
        if (!(packet->source == c.remote))
            continue;
        if (auto challenge = decodeMatchChallenge(packet->bytes)) {
            if (!c.received) {
                c.token = *challenge;
                c.challengeTime = Clock::now();
                c.socket.send(c.remote, encodeMatchAccept(c.token));
            }
            continue;
        }
        auto snapshot = decodeMatch(packet->bytes);
        if (!snapshot || !c.token || snapshot->token != c.token)
            continue;
        if (c.received &&
            (snapshot->frame.tick < c.frame.tick ||
             (snapshot->frame.tick == c.frame.tick &&
              static_cast<unsigned>(snapshot->frame.phase) < static_cast<unsigned>(c.frame.phase))))
            continue;
        if (!newest || snapshot->frame.tick > newest->frame.tick ||
            (snapshot->frame.tick == newest->frame.tick &&
             static_cast<unsigned>(snapshot->frame.phase) > static_cast<unsigned>(newest->frame.phase)))
            newest = std::move(snapshot);
    }
    if (newest) {
        c.lastSeen = Clock::now();
        c.received = true;
        c.slot = newest->slot;
        const auto map = newest->frame.rules.map;
        if (!c.arena || c.frame.rules.map != map)
            c.arena = std::make_unique<MatchArena>(map);
        c.frame = std::move(newest->frame);
        c.predicted = c.frame.players[c.slot];
        while (!c.pending.empty() && c.pending.front().command.tick <= c.predicted.acknowledged)
            c.pending.pop_front();
        if (c.frame.phase == MatchPhase::Finished) {
            c.pending.clear();
            return;
        }
        for (const auto &input : c.pending)
            c.arena->simulate(c.predicted, input.command);
    }
    if (Clock::now() - c.lastSeen > std::chrono::seconds(10))
        throw std::runtime_error("Match server timed out");
    if (c.frame.phase == MatchPhase::Waiting) {
        const auto now = Clock::now();
        if (now - c.lastRetry >= std::chrono::milliseconds(250)) {
            if (c.token && (c.received || now - c.challengeTime < std::chrono::seconds(2)))
                c.socket.send(c.remote, encodeMatchAccept(c.token));
            else {
                if (!c.received)
                    c.token = 0;
                c.socket.send(c.remote, encodeMatchJoin());
            }
            c.lastRetry = now;
        }
        return;
    }
    if (command && c.pending.size() < 120) {
        command->tick = ++c.nextInput;
        sanitizeMatchCommand(*command);
        c.pending.push_back({*command, wasPlaying ? observedTick : c.frame.tick});
        c.arena->simulate(c.predicted, *command);
    }
    if (!c.pending.empty()) {
        // Carry the first gap as well as fresh inputs, even if the RTT exceeds eight ticks.
        const auto recent = std::min<std::size_t>(7, c.pending.size() - 1);
        std::vector<MatchInput> inputs;
        inputs.reserve(1 + recent);
        inputs.push_back(c.pending.front());
        inputs.insert(inputs.end(), c.pending.end() - recent, c.pending.end());
        c.socket.send(c.remote, encodeMatchCommands(c.slot, c.token, inputs));
    }
}
} // namespace nemisis::net
