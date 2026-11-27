#include "nemisis/net/FacilityMatch.hpp"

#include <charconv>
#include <exception>
#include <iostream>
#include <string_view>
#include <vector>

namespace nemisis::net {
namespace {
bool parseBounded(
    std::string_view value, std::uint32_t& result, std::uint32_t minimum, std::uint32_t maximum) {
    const auto end = value.data() + value.size();
    const auto parsed = std::from_chars(value.data(), end, result);
    return parsed.ec == std::errc{} && parsed.ptr == end && result >= minimum && result <= maximum;
}

using Endpoint = novacore::net::DatagramEndpoint;
using MatchRules = nemisis::net::MatchRules;

std::optional<Endpoint> parseRemote(std::string_view value, std::string& host) {
    const auto divider = value.find(':');
    host = divider == std::string_view::npos ? "127.0.0.1" : std::string(value.substr(0, divider));
    const auto portText = divider == std::string_view::npos ? value : value.substr(divider + 1);
    std::uint32_t port = 0;
    if (!parseBounded(portText, port, 1, 65535)) return {};
    auto endpoint = Endpoint::parse(host, static_cast<std::uint16_t>(port));
    if (!endpoint || !endpoint->address) return {};
    return endpoint;
}

std::optional<MatchRules> parseRound(std::string_view value) {
    std::array<std::string_view, 5> fields;
    for (unsigned i = 0; i < fields.size(); ++i) {
        const auto separator = value.find(',');
        if ((separator == std::string_view::npos) != (i + 1 == fields.size())) return {};
        fields[i] = value.substr(0, separator);
        if (separator != std::string_view::npos) value.remove_prefix(separator + 1);
    }
    MatchRules rules;
    if (fields[0] == "dev-range") rules.map = nemisis::net::MatchMap::DevRange;
    else if (fields[0] != "outer-facility") return {};
    if (fields[1] == "tdm") rules.mode = nemisis::net::MatchMode::TeamDeathmatch;
    else if (fields[1] != "duel") return {};
    std::uint32_t seconds = 0, size = 0;
    if (!parseBounded(fields[2], size, 1, nemisis::net::maxMatchPlayers / 2) ||
        !parseBounded(fields[3], rules.scoreLimit, 1, 10000) ||
        !parseBounded(fields[4], seconds, 1, 3600)) return {};
    rules.teamSize = size;
    rules.durationTicks = seconds * 60;
    if (!nemisis::net::validMatchRules(rules)) return {};
    return rules;
}
} // namespace

std::optional<int> runMatchCommandLine(int argc, char** argv, bool dedicated, std::string& joinHost,
                                       std::uint16_t& joinPort) {
    bool singleRound = false;
    std::optional<std::uint16_t> serverPort;
    std::optional<Endpoint> probe;
    std::optional<std::string_view> bindAddress;
    std::vector<MatchRules> rotation;
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument(argv[i]);
        if (argument.starts_with("--match-server=") || argument.starts_with("--match-test-server=")) {
            std::uint32_t port = 0;
            if (serverPort || probe || joinPort ||
                !parseBounded(argument.substr(argument.find('=') + 1), port, 0, 65535)) {
                std::cerr << "Specify one match role and a UDP port in 0..65535\n";
                return 2;
            }
            serverPort = static_cast<std::uint16_t>(port);
            singleRound = argument.starts_with("--match-test-server=");
        } else if (argument.starts_with("--round=")) {
            const auto rules = parseRound(argument.substr(8));
            if (!rules || rotation.size() >= 64) {
                std::cerr << "Expected --round=MAP,MODE,TEAM_SIZE,SCORE_LIMIT,SECONDS (up to 64 rounds)\n";
                return 2;
            }
            rotation.push_back(*rules);
        } else if (argument.starts_with("--bind=")) {
            bindAddress = argument.substr(7);
        } else if (argument.starts_with("--match-probe=") || argument.starts_with("--join-local=") ||
                   argument.starts_with("--join=")) {
            const auto value = argument.substr(argument.find('=') + 1);
            const auto endpoint = parseRemote(value, joinHost);
            if (!endpoint || probe || serverPort || joinPort ||
                (argument.starts_with("--join-local=") && value.find(':') != std::string_view::npos)) {
                std::cerr << "Expected one match destination, e.g. --join=192.168.1.20:27015\n";
                return 2;
            }
            if (argument.starts_with("--match-probe=")) probe = *endpoint;
            else if (dedicated) return 2;
            else {
                joinPort = endpoint->port;
            }
        } else if (dedicated || argument.starts_with("--match-") || argument.starts_with("--round") ||
                   argument.starts_with("--join") || argument.starts_with("--bind")) {
            std::cerr << "Unknown match option " << argument << '\n';
            return 2;
        }
    }
    if (dedicated && !serverPort && !probe) serverPort = 27015;
    if (!serverPort && (bindAddress || !rotation.empty())) {
        std::cerr << "--bind and --round require a match server\n";
        return 2;
    }
    try {
        if (serverPort) {
            const auto bind = Endpoint::parse(bindAddress.value_or("127.0.0.1"), *serverPort);
            if (!bind || (singleRound && rotation.size() > 1)) {
                std::cerr << "Invalid IPv4 bind address or single-round configuration\n";
                return 2;
            }
            if (rotation.empty()) {
                rotation.emplace_back();
                if (singleRound) rotation.front().scoreLimit = 2;
            }
            return nemisis::net::runFacilityMatchServer(*bind, rotation, singleRound);
        }
        if (probe) return nemisis::net::runFacilityMatchProbe(*probe);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return std::nullopt;
}
}