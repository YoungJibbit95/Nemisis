#include "nemisis/GameApp.hpp"

#include "novacore/core/Application.hpp"

#include <string_view>
#include <iostream>
#include <exception>

int main(int argc, char** argv) {
    novacore::core::ApplicationDesc desc{};
    desc.name = "nemisis_game";
    desc.fixedTickHz = 60.0;
    desc.maxFrames = 0;

    nemisis::game::GameAppOptions gameOptions{};
    bool smokeTest = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument(argv[i]);
        if (argument == "--ui-showcase") {
#if NEMISIS_HAS_RMLUI
            gameOptions.uiShowcase = true;
            gameOptions.autoEnterDevRange = false;
            gameOptions.lockDevRange = false;
#else
            std::cerr << "--ui-showcase requires -DNEMISIS_ENABLE_RMLUI=ON\n";
            return 2;
#endif
        } else if (argument.starts_with("--ui-size=")) {
            const auto size = argument.substr(10);
            if (size == "1280x720") { gameOptions.uiPreviewWidth = 1280; gameOptions.uiPreviewHeight = 720; }
            else if (size == "1920x1080") { gameOptions.uiPreviewWidth = 1920; gameOptions.uiPreviewHeight = 1080; }
            else if (size == "2560x1440") { gameOptions.uiPreviewWidth = 2560; gameOptions.uiPreviewHeight = 1440; }
            else { std::cerr << "UI size must be 1280x720, 1920x1080 or 2560x1440\n"; return 2; }
        } else if (argument == "--smoke-test") {
            smokeTest = true;
        } else if (argument == "--project-rifle-ready") {
#if defined(NEMISIS_PROJECT_FPS_PREVIEW) && defined(NEMISIS_ENABLE_HAND_IK)
            gameOptions.projectRifleReady = true;
            gameOptions.projectFPSPreview = true;
            gameOptions.firstPersonHandIK = true;
            gameOptions.autoEnterDevRange = true;
            gameOptions.lockDevRange = true;
#else
            std::cerr << "Project Rifle Ready requires Project FPS candidate and hand IK build options\n";
            return 2;
#endif
        } else if (argument == "--project-fps-look-down") {
            gameOptions.projectFPSLookDown = true;
        } else if (argument == "--project-fps-preview") {
#if defined(NEMISIS_PROJECT_FPS_PREVIEW)
            gameOptions.projectFPSPreview = true;
            gameOptions.autoEnterDevRange = true;
            gameOptions.lockDevRange = true;
#else
            std::cerr << "Project FPS preview requires -DNEMISIS_BUILD_PROJECT_FPS_CANDIDATE=ON\n";
            return 2;
#endif
        } else if (argument == "--first-person-hand-ik") {
#if defined(NEMISIS_ENABLE_HAND_IK)
            gameOptions.firstPersonHandIK = true;
#else
            std::cerr << "Hand IK requires -DNEMISIS_ENABLE_HAND_IK=ON at configure time\n";
            return 2;
#endif
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

    if (gameOptions.uiShowcase) {
        if (gameOptions.runMenuFlowSmoke || gameOptions.projectFPSPreview || gameOptions.firstPersonHandIK) {
            std::cerr << "UI showcase cannot be combined with other preview profiles\n"; return 2;
        }
        if (!gameOptions.preferVulkanRenderer) {
            std::cerr << "--ui-showcase requires the Vulkan renderer\n"; return 2;
        }
        gameOptions.requireVulkanRenderer = true;
        gameOptions.autoEnterDevRange = false;
        gameOptions.lockDevRange = false;
    }
    if (smokeTest) {
        desc.maxFrames = gameOptions.runMenuFlowSmoke ? 44 :
            (gameOptions.firstPersonHandIK || gameOptions.projectFPSPreview) ? 60 : 5;
    }

    if (gameOptions.projectFPSLookDown && !gameOptions.projectFPSPreview) {
        std::cerr << "--project-fps-look-down requires --project-fps-preview\n";
        return 2;
    }
    if (gameOptions.projectFPSPreview && gameOptions.firstPersonHandIK && !gameOptions.projectRifleReady) {
        std::cerr << "Project hand IK is blocked: five weighted deform segments, nonuniform animated scales "
                     "and unapproved palm frames. Run --project-fps-preview without --first-person-hand-ik.\n";
        return 2;
    }

    nemisis::game::GameApp game(gameOptions);
    novacore::core::Application app(desc);
    try {
        app.run(game);
    } catch (const std::exception& error) {
        std::cerr << "Nemisis startup/runtime failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
