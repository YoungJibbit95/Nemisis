#include "nemisis/GameApp.hpp"
#include "nemisis/dev/AuthoredWorld.hpp"

#include "novacore/core/Application.hpp"

#include <charconv>
#include <cstdint>
#include <string_view>
#include <iostream>
#include <exception>

int main(int argc, char** argv) {
    // Isolated local map profile. Other preview/network flags cannot override it.
    for (int i=1; i<argc; ++i) {
        if (std::string_view(argv[i]) != "--map") continue;
        const bool check = argc == 4 && std::string_view(argv[3]) == "--check-map";
        if (i != 1 || (argc != 3 && !check) || std::string_view(argv[2]).empty()) {
            std::cerr << "Usage: nemisis_game --map FILE [--check-map]\n"; return 2;
        }
        try {
            if (check) {
                nemisis::assets::GameAssetCatalog catalog;
                const auto loaded = catalog.loadFromJson("configs/assets/nemisis_assets.json");
                if (!loaded.ok()) { std::cerr << "Asset catalog unavailable: " << loaded.errors.front() << '\n'; return 1; }
                nemisis::dev::GreyboxWorld world;
                if (auto error = nemisis::dev::loadAuthoredWorld(argv[2],catalog,world)) {
                    std::cerr << *error << '\n'; return 1;
                }
                std::cout << "Authored world validated (CPU): " << world.id << " boxes=" << world.primitives.size()
                    << " spawn=" << world.playerSpawnId << '\n';
                return 0;
            }
            nemisis::game::GameAppOptions options{};
            options.authoredMap=argv[2]; options.autoEnterDevRange=true; options.lockDevRange=true;
            options.preferVulkanRenderer=true; options.requireVulkanRenderer=true;
            nemisis::game::GameApp game(options);
            novacore::core::Application app({"nemisis_authored_map",60.0,0});
            app.run(game);
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "Authored world playtest failed: " << error.what() << '\n'; return 1;
        }
    }
    novacore::core::ApplicationDesc desc{};
    desc.name = "nemisis_game";
    desc.fixedTickHz = 60.0;
    desc.maxFrames = 0;

    nemisis::game::GameAppOptions gameOptions{};
    gameOptions.fullGame = argc == 1;
    std::uint32_t measuredFrames = 60;
    bool smokeTest = false;
    if (const auto result = nemisis::net::runMatchCommandLine(argc, argv, false,
            gameOptions.matchHost, gameOptions.localMatchPort)) return *result;
    if (gameOptions.localMatchPort) {
        gameOptions.fullGame = true;
        gameOptions.startInFacility = true;
    }
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument(argv[i]);
        if (argument == "--private-match-smoke") {
            gameOptions.fullGame = true;
            gameOptions.runPrivateMatchSmoke = true;
            smokeTest = true;
        } else if (argument == "--game-deploy-smoke-test") {
            gameOptions.fullGame = true;
            gameOptions.runDeploymentSmoke = true;
            smokeTest = true;
        } else if (argument == "--game" || argument == "--outer-facility") {
            gameOptions.fullGame = true;
            if (argument == "--outer-facility") { gameOptions.startInFacility = true; }
        } else if (argument == "--ui-showcase") {
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
        } else if (argument == "--profile-range" || argument.starts_with("--profile-range=")) {
            if (argument.starts_with("--profile-range=")) {
                const auto count = argument.substr(std::string_view("--profile-range=").size());
                const auto end = count.data() + count.size();
                const auto parsed = std::from_chars(count.data(), end, measuredFrames);
                if (parsed.ec != std::errc{} || parsed.ptr != end || measuredFrames == 0) {
                    std::cerr << "--profile-range expects a positive frame count\n";
                    return 2;
                }
            }
            gameOptions.profileRange = true;
        } else if (argument == "--physics-props") {
            gameOptions.physicsProps = true;
        } else if (argument == "--directional-shadows") {
            gameOptions.directionalShadows = true;
        } else if (argument == "--material-textures") {
            gameOptions.materialTextures = true;
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

    if (gameOptions.fullGame) {
        gameOptions.materialTextures = true;
        gameOptions.physicsProps = !gameOptions.localMatchPort && novacore::physics::RigidBodyWorld::backendAvailable();
#if defined(NEMISIS_PROJECT_FPS_PREVIEW) && defined(NEMISIS_ENABLE_HAND_IK)
        gameOptions.projectRifleReady = true;
        gameOptions.projectFPSPreview = true;
        gameOptions.firstPersonHandIK = true;
#endif
    }
    if (gameOptions.uiShowcase) {
        if (gameOptions.runMenuFlowSmoke || gameOptions.projectFPSPreview || gameOptions.firstPersonHandIK || gameOptions.physicsProps || gameOptions.directionalShadows || gameOptions.materialTextures) {
            std::cerr << "UI showcase cannot be combined with other preview profiles\n"; return 2;
        }
        if (!gameOptions.preferVulkanRenderer) {
            std::cerr << "--ui-showcase requires the Vulkan renderer\n"; return 2;
        }
        gameOptions.requireVulkanRenderer = true;
        gameOptions.autoEnterDevRange = false;
        gameOptions.lockDevRange = false;
    }
    if (gameOptions.physicsProps) {
        if (!novacore::physics::RigidBodyWorld::backendAvailable()) {
            std::cerr << "--physics-props requires NovaCore configured with -DNOVACORE_ENABLE_JOLT=ON\n";
            return 2;
        }
        gameOptions.autoEnterDevRange = true;
        gameOptions.lockDevRange = true;
    }
    if (gameOptions.directionalShadows || gameOptions.materialTextures) {
        gameOptions.autoEnterDevRange = true;
        gameOptions.lockDevRange = true;
    }
    if (smokeTest) {
        desc.maxFrames = (gameOptions.runDeploymentSmoke || gameOptions.runPrivateMatchSmoke) ? 0 : gameOptions.runMenuFlowSmoke ? 58 : gameOptions.physicsProps ? 120 :
            (gameOptions.firstPersonHandIK || gameOptions.projectFPSPreview) ? 60 : 5;
    }
    if (gameOptions.profileRange) {
        if (gameOptions.uiShowcase || smokeTest || gameOptions.localMatchPort) {
            std::cerr << "--profile-range requires a dedicated Dev Range run\n";
            return 2;
        }
        gameOptions.requireVulkanRenderer = true;
        gameOptions.preferVulkanRenderer = true;
        gameOptions.autoEnterDevRange = true;
        gameOptions.lockDevRange = true;
        desc.maxFrames = nemisis::game::kRangeProfileWarmupFrames + measuredFrames;
    }

    if (gameOptions.projectFPSLookDown && !gameOptions.projectFPSPreview) {
        std::cerr << "--project-fps-look-down requires --project-fps-preview\n";
        return 2;
    }
    if (gameOptions.fullGame) {
        gameOptions.autoEnterDevRange = gameOptions.profileRange && !gameOptions.startInFacility;
        gameOptions.lockDevRange = false;
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
