#include "nemisis/net/FacilityMatch.hpp"

int main(int argc, char** argv) {
    std::string unusedHost;
    std::uint16_t unusedPort = 0;
    return nemisis::net::runMatchCommandLine(argc, argv, true, unusedHost, unusedPort).value_or(2);
}
