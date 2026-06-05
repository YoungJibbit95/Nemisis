#include "nemisis/GameApp.hpp"

#include "novacore/core/Application.hpp"

#include <string_view>

int main(int argc, char** argv) {
    nemisis::game::GameApp game;

    novacore::core::ApplicationDesc desc{};
    desc.name = "nemisis_game";
    desc.fixedTickHz = 60.0;
    desc.maxFrames = 0;

    bool smokeTest = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--smoke-test") {
            smokeTest = true;
        }
    }

    if (smokeTest) {
        desc.maxFrames = 5;
    }

    novacore::core::Application app(desc);
    app.run(game);
    return 0;
}
