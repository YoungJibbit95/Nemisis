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
        } else if (argument == "--menu") {
            gameOptions.autoEnterDevRange = false;
            gameOptions.lockDevRange = false;
        } else if (argument == "--sdl-debug" || argument == "--no-vulkan") {
            gameOptions.preferVulkanRenderer = false;
            gameOptions.requireVulkanRenderer = false;
            gameOptions.autoEnterDevRange = false;
            gameOptions.lockDevRange = false;
        } else if (argument == "--vulkan") {
            gameOptions.preferVulkanRenderer = true;
            gameOptions.requireVulkanRenderer = true;
        } else if (argument == "--dev-range") {
            gameOptions.autoEnterDevRange = true;
            gameOptions.lockDevRange = true;
        } else if (argument == "--vulkan-smoke-test") {
            smokeTest = true;
            gameOptions.preferVulkanRenderer = true;
            gameOptions.requireVulkanRenderer = true;
            gameOptions.autoEnterDevRange = false;
            gameOptions.lockDevRange = false;
        } else if (argument == "--vulkan-dev-range-smoke-test") {
            smokeTest = true;
            gameOptions.preferVulkanRenderer = true;
            gameOptions.requireVulkanRenderer = true;
            gameOptions.autoEnterDevRange = true;
            gameOptions.lockDevRange = true;
        } else if (argument == "--menu-flow-smoke-test") {
            smokeTest = true;
            gameOptions.preferVulkanRenderer = true;
            gameOptions.requireVulkanRenderer = true;
            gameOptions.autoEnterDevRange = false;
            gameOptions.lockDevRange = false;
            gameOptions.runMenuFlowSmoke = true;
        } else if (argument == "--sdl-debug-smoke-test") {
            smokeTest = true;
            gameOptions.preferVulkanRenderer = false;
            gameOptions.requireVulkanRenderer = false;
            gameOptions.autoEnterDevRange = false;
            gameOptions.lockDevRange = false;
        }
    }

    if (smokeTest) {
        desc.maxFrames = gameOptions.runMenuFlowSmoke ? 44 : 5;
    }

    nemisis::game::GameApp game(gameOptions);
    novacore::core::Application app(desc);
    app.run(game);
    return 0;
}
