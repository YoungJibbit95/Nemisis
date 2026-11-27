#include "nemisis/net/FacilityMatch.hpp"
#include "FacilityMatchCodec.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <random>
#include <stdexcept>
#include <thread>

namespace nemisis::net {
namespace {
using Clock = std::chrono::steady_clock;
using Endpoint = novacore::net::DatagramEndpoint;
using Socket = novacore::net::DatagramSocket;
using namespace std::chrono_literals;
constexpr auto stepTime = std::chrono::nanoseconds(1'000'000'000 / 60);
constexpr unsigned inputWindow = 120;

const char* mapName(MatchMap map) { return map == MatchMap::OuterFacility ? "outer-facility" : "dev-range"; }
const char* modeName(MatchMode mode) { return mode == MatchMode::Duel ? "duel" : "tdm"; }

void printResult(const MatchFrame& frame) {
    std::cout << "MATCH_RESULT " << frame.tick << ' ' << frame.remaining;
    const auto players = std::span(frame.players).first(2 * frame.rules.teamSize);
    for (const auto& p : players)
        std::cout << ' ' << p.score << ' ' << p.health.health << ' ' << p.weapon.ammoInMagazine
            << ' ' << p.hits << ' ' << p.acknowledged << ' ' << p.movement.position.x
            << ' ' << p.movement.position.y << ' ' << p.movement.position.z;
    for (const auto& p : players)
        std::cout << ' ' << p.weapon.weaponId << ' ' << p.holstered.ammoInMagazine;
    std::cout << std::endl;
}

struct Challenge {
    Endpoint endpoint{};
    std::uint64_t token = 0;
    Clock::time_point expires{};
};

struct Peer {
    Endpoint endpoint{};
    std::uint64_t token = 0;
    Clock::time_point lastSeen{};
    std::map<std::uint64_t, MatchInput> inputs;
    unsigned firstAckSamples = 0;
    bool ready = false;
};

struct RoundStats {
    std::array<unsigned, 1000> histogram{};
    std::uint64_t totalUs = 0, snapshotBytes = 0;
    unsigned ticks = 0, maxUs = 0, overBudget = 0, datagrams = 0;
    std::size_t maxDatagram = 0;

    void record(Clock::duration work) {
        const auto us = static_cast<unsigned>(std::chrono::duration_cast<std::chrono::microseconds>(work).count());
        ++ticks;
        totalUs += us;
        maxUs = std::max(maxUs, us);
        if (work > stepTime) ++overBudget;
        ++histogram[std::min<std::size_t>(us / 100, histogram.size() - 1)];
    }
    unsigned percentile(unsigned percent) const {
        unsigned cumulative = 0;
        for (unsigned i = 0; i < histogram.size(); ++i) {
            cumulative += histogram[i];
            if (cumulative * 100 >= ticks * percent)
                return i == histogram.size() - 1 ? maxUs : (i + 1) * 100;
        }
        return maxUs;
    }
    void print() const {
        std::cout << "MATCH_STATS ticks=" << ticks << " avg_us=" << totalUs / std::max(1U, ticks)
            << " p50_us=" << percentile(50) << " p95_us=" << percentile(95) << " p99_us=" << percentile(99)
            << " max_us=" << maxUs << " over_budget=" << overBudget
            << " snapshot_bytes=" << snapshotBytes << " datagrams=" << datagrams
            << " max_datagram=" << maxDatagram << std::endl;
    }
};

class HostedRound {
public:
    HostedRound(Socket& socket, const MatchRules& rules, bool diagnostic)
        : socket_(socket), arena_(rules.map), peers_(rules.teamSize * 2), diagnostic_(diagnostic) {
        frame_.rules = rules;
        frame_.remaining = rules.durationTicks;
        for (unsigned i = 0; i < peers_.size(); ++i) arena_.spawn(frame_.players[i], i);
    }

    void run() {
        std::cout << "MATCH_ROUND " << mapName(frame_.rules.map) << ' ' << modeName(frame_.rules.mode)
            << ' ' << frame_.rules.teamSize << std::endl;
        auto next = Clock::now();
        unsigned pulses = 0, terminalTicks = 0;
        for (;;) {
            const auto start = Clock::now();
            for (unsigned budget = 0; budget < 128; ++budget) {
                const auto packet = socket_.receive();
                if (!packet) break;
                receive(packet->source, packet->bytes, start);
            }
            for (auto& peer : peers_) {
                if (!peer.token || start - peer.lastSeen < 8s) continue;
                if (frame_.phase == MatchPhase::Waiting) peer = {};
                else if (frame_.phase == MatchPhase::Playing) {
                    frame_.interrupted = true;
                    frame_.phase = MatchPhase::Finished;
                    std::cout << "MATCH_DISCONNECT slot=" << (&peer - peers_.data()) << std::endl;
                }
            }
            if (frame_.phase == MatchPhase::Waiting &&
                std::ranges::all_of(peers_, [](const Peer& peer) { return peer.token != 0 && peer.ready; }))
                frame_.phase = MatchPhase::Playing;

            const bool simulated = frame_.phase == MatchPhase::Playing;
            if (simulated) simulate();
            const unsigned interval = frame_.phase == MatchPhase::Waiting ? 15U :
                frame_.phase == MatchPhase::Finished ? 4U : 2U;
            if (pulses % interval == 0 || (simulated && frame_.tick <= 4) ||
                (frame_.phase == MatchPhase::Finished && terminalTicks == 0)) broadcast(simulated);
            if (simulated) stats_.record(Clock::now() - start);
            if (frame_.phase == MatchPhase::Finished && ++terminalTicks == 40) break;
            ++pulses;
            next += stepTime;
            if (next < start - stepTime * 4) next = start;
            std::this_thread::sleep_until(next);
        }
        printResult(frame_);
        stats_.print();
    }

private:
    void receive(Endpoint source, const std::vector<std::uint8_t>& bytes, Clock::time_point now) {
        auto peer = std::ranges::find(peers_, source, &Peer::endpoint);
        if (peer == peers_.end()) {
            if (frame_.phase == MatchPhase::Waiting) handshake(source, bytes, now);
            return;
        }
        if (frame_.phase == MatchPhase::Waiting) {
            if (decodeMatchReady(bytes) == peer->token) {
                peer->lastSeen = now;
                peer->ready = true;
            } else if (decodeMatchAccept(bytes) == peer->token) peer->lastSeen = now;
            else if (isMatchJoin(bytes)) socket_.send(source, encodeMatchChallenge(peer->token));
            return;
        }
        if (frame_.phase != MatchPhase::Playing) return;
        const auto packet = decodeMatchCommands(bytes);
        const auto slot = static_cast<unsigned>(peer - peers_.begin());
        if (!packet || packet->slot != slot || packet->token != peer->token) return;
        peer->lastSeen = now;
        const auto ack = frame_.players[slot].acknowledged;
        for (auto input : packet->inputs) {
            const auto tick = input.command.tick;
            if (tick <= ack || tick - ack > inputWindow) continue;
            sanitizeMatchCommand(input.command);
            peer->inputs.try_emplace(tick, input);
        }
    }

    void handshake(Endpoint source, const std::vector<std::uint8_t>& bytes, Clock::time_point now) {
        auto challenge = std::ranges::find(challenges_, source, &Challenge::endpoint);
        if (isMatchJoin(bytes)) {
            if (challenge == challenges_.end() || challenge->expires <= now) {
                challenge = std::ranges::find_if(challenges_, [now](const Challenge& c) { return c.expires <= now; });
                if (challenge == challenges_.end()) return;
                const auto token = (std::uint64_t{entropy_()} << 32) | entropy_();
                *challenge = {source, token ? token : 1, now + 3s};
            }
            socket_.send(source, encodeMatchChallenge(challenge->token));
            return;
        }
        if (challenge == challenges_.end() || challenge->expires <= now ||
            decodeMatchAccept(bytes) != challenge->token) return;
        const auto vacant = std::ranges::find(peers_, std::uint64_t{0}, &Peer::token);
        if (vacant == peers_.end()) return;
        vacant->endpoint = source;
        vacant->token = challenge->token;
        vacant->lastSeen = now;
        *challenge = {};
    }

    void simulate() {
        ++frame_.tick;
        --frame_.remaining;
        std::array<std::optional<weapons::ShotTraceResult>, maxMatchPlayers> shots;
        std::array<std::uint32_t, maxMatchPlayers> viewed{};
        for (unsigned i = 0; i < peers_.size(); ++i) {
            auto& actor = frame_.players[i];
            if (actor.respawn && --actor.respawn == 0) {
                arena_.spawn(actor, i);
                actor.spawnTick = frame_.tick;
            }
            MatchInput input{};
            auto& queue = peers_[i].inputs;
            if (!queue.empty() && queue.begin()->first == actor.acknowledged + 1) {
                input = queue.begin()->second;
                actor.acknowledged = input.command.tick;
                queue.erase(queue.begin());
            }
            shots[i] = arena_.simulate(actor, input.command);
            viewed[i] = input.observedTick;
            if (diagnostic_ && actor.acknowledged == 1 && peers_[i].firstAckSamples < 8) {
                ++peers_[i].firstAckSamples;
                std::cout << "MATCH_AUTH slot=" << i << " ack=1 z=" << actor.movement.position.z << " server=" << frame_.tick << std::endl;
            }
        }
        // Sample after everyone moves so every shot can trade on the same simulation tick.
        arena_.record(frame_);
        for (unsigned i = 0; i < peers_.size(); ++i)
            if (shots[i]) arena_.resolveShot(frame_, i, *shots[i], viewed[i]);
        if (!frame_.remaining || std::ranges::max(frame_.teamScores) >= frame_.rules.scoreLimit)
            frame_.phase = MatchPhase::Finished;
    }

    void broadcast(bool measure) {
        for (unsigned i = 0; i < peers_.size(); ++i) {
            if (!peers_[i].token) continue;
            const auto bytes = encodeMatch(frame_, i, peers_[i].token);
            socket_.send(peers_[i].endpoint, bytes);
            if (measure) {
                stats_.snapshotBytes += bytes.size();
                ++stats_.datagrams;
                stats_.maxDatagram = std::max(stats_.maxDatagram, bytes.size());
            }
        }
    }

    Socket& socket_;
    MatchArena arena_;
    std::vector<Peer> peers_;
    MatchFrame frame_;
    std::array<Challenge, 64> challenges_{};
    std::random_device entropy_;
    RoundStats stats_;
    bool diagnostic_;
};
}

int runFacilityMatchServer(Endpoint bind, std::span<const MatchRules> rotation, bool singleRound) {
    if (rotation.empty() || !std::ranges::all_of(rotation, validMatchRules))
        throw std::invalid_argument("Invalid match rotation");
    Socket socket(bind);
    std::cout << "MATCH_LISTEN " << socket.port() << std::endl;
    do {
        for (const auto& rules : rotation) {
            HostedRound(socket, rules, singleRound).run();
            if (singleRound) return 0;
        }
    } while (true);
}

int runFacilityMatchProbe(Endpoint remote) {
    FacilityMatchClient client(remote);
    auto next = Clock::now();
    const auto deadline = next + 45s;
    bool tracedPrediction = false, tracedAck = false, sawElimination = false, sawRespawn = false;
    while (Clock::now() < deadline) {
        player::PlayerInputCommand command{};
        if (!tracedPrediction) command.move.y = 1;
        const auto& previous = client.authoritative();
        if (previous.phase == MatchPhase::Playing && previous.rules.mode == MatchMode::TeamDeathmatch) {
            const auto route = (previous.tick / 20 + client.slot()) % 4;
            command.move.x = route < 2 ? 0.6F : -0.6F;
            command.move.y = route % 2 ? 0.4F : -0.4F;
            command.adsHeld = true;
            command.fireHeld = (previous.tick + client.slot()) % 12 < 8;
            command.reloadPressed = client.predicted().weapon.ammoInMagazine == 0;
        } else if (previous.phase == MatchPhase::Playing && client.slot() == 0) {
            command.adsHeld = true;
            const auto& frame = client.authoritative();
            command.switchWeaponSidearmPressed = frame.players[0].score >= 1;
            command.fireHeld = !frame.players[1].health.eliminated;
            command.reloadPressed = client.predicted().weapon.ammoInMagazine == 0;
        }
        // Hold additional inputs until the first acknowledgement is applied.
        client.tick(tracedPrediction && !tracedAck ? std::nullopt : std::optional{command});
        const auto& frame = client.authoritative();
        if (frame.phase == MatchPhase::Waiting) client.confirmMapReady();
        if (frame.phase != MatchPhase::Waiting && !tracedPrediction) {
            std::cout << "MATCH_RULES " << mapName(frame.rules.map) << ' ' << modeName(frame.rules.mode)
                << ' ' << frame.rules.teamSize << "\nMATCH_SLOT " << client.slot()
                << "\nMATCH_PREDICT slot=" << client.slot() << " input=1 z=" << client.predicted().movement.position.z << std::endl;
            tracedPrediction = true;
        }
        const auto& self = frame.players[client.slot()];
        if (frame.players[1].health.eliminated) sawElimination = true;
        if (sawElimination && !sawRespawn && !frame.players[1].health.eliminated) {
            sawRespawn = true;
            std::cout << "MATCH_RESPAWN slot=1" << std::endl;
        }
        if (self.acknowledged && !tracedAck) {
            std::cout << "MATCH_RECONCILE slot=" << client.slot() << " ack=" << self.acknowledged << " z=" << self.movement.position.z << " server=" << frame.tick << std::endl;
            tracedAck = true;
        }
        if (frame.phase == MatchPhase::Finished) {
            printResult(frame);
            return tracedPrediction && self.acknowledged ? 0 : 3;
        }
        next += stepTime;
        std::this_thread::sleep_until(next);
    }
    throw std::runtime_error("Scripted match did not complete");
}
}
