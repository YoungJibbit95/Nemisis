#include "nemisis/GameApp.hpp"

#include "novacore/core/Application.hpp"

#include <string_view>

int main(int argc, char** argv) {
    novacore::core::ApplicationDesc desc{};
    desc.name = "nemisis_game";
    desc.fixedTickHz = 60.0;
    desc.maxFrames = 0;

    nemisis::game::GameAppOptions gameOptions{};
    bool smokeTest = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument(argv[i]);
        if (argument == "--smoke-test") {
            smokeTest = true;
        } else if (argument == "--vulkan") {
            gameOptions.preferVulkanRenderer = true;
        } else if (argument == "--vulkan-smoke-test") {
            smokeTest = true;
            gameOptions.preferVulkanRenderer = true;
        }
    }

    if (smokeTest) {
        desc.maxFrames = 5;
    }

    nemisis::game::GameApp game(gameOptions);
    novacore::core::Application app(desc);
    app.run(game);
    return 0;
}
