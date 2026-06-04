#include "nemisis/GameApp.hpp"

#include "novacore/core/Application.hpp"

int main() {
    nemisis::game::GameApp game;

    novacore::core::ApplicationDesc desc{};
    desc.name = "nemisis_game";
    desc.fixedTickHz = 60.0;
    desc.maxFrames = 0;

#if !NOVACORE_HAS_SDL3
    desc.maxFrames = 5;
#endif

    novacore::core::Application app(desc);
    app.run(game);
    return 0;
}

