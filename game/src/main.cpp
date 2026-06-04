#include "fps/core/Application.hpp"
#include "fps/core/Log.hpp"
#include "fps/ecs/World.hpp"
#include "fps/platform/Input.hpp"
#include "fps/platform/Window.hpp"
#include "fps/render/Renderer.hpp"

#include <array>

namespace {

class GameApp final : public novacore::core::IApplicationDelegate {
public:
    void onStartup() override {
        novacore::platform::WindowDesc windowDesc{};
        windowDesc.title = "Nemesis - M1 Thin Spine";
        windowDesc.width = 1280;
        windowDesc.height = 720;
        windowDesc.preferVulkan = true;

        window_.create(windowDesc);

        novacore::render::RendererCreateInfo rendererInfo{};
        renderer_.create(window_, rendererInfo);

        const auto camera = world_.createEntity();
        world_.addComponent(camera, novacore::ecs::NameComponent{"main_camera"});
        world_.addComponent(camera, novacore::ecs::TransformComponent{});
        world_.addComponent(camera, novacore::ecs::CameraComponent{});

        novacore::core::logInfo("game", "Sandbox camera entity created");
    }

    void onShutdown() override {
        renderer_.shutdown();
        window_.shutdown();
    }

    void onFixedTick(const novacore::core::FrameContext& context) override {
        (void)context;
    }

    void onFrame(const novacore::core::FrameContext& context) override {
        (void)context;
        window_.pollEvents(input_);

        novacore::render::RenderFrameInfo frameInfo{};
        frameInfo.clearColor = std::array<float, 4>{0.025F, 0.035F, 0.055F, 1.0F};
        renderer_.beginFrame(frameInfo);
        renderer_.endFrame();
    }

    bool shouldQuit() const override {
        return window_.shouldClose();
    }

    bool isHeadless() const {
        return window_.isHeadless();
    }

private:
    novacore::platform::InputSystem input_;
    novacore::platform::Window window_;
    novacore::render::Renderer renderer_;
    novacore::ecs::World world_;
};

} // namespace

int main() {
    GameApp game;

    novacore::core::ApplicationDesc desc{};
    desc.name = "nemesis_game";
    desc.fixedTickHz = 60.0;
    desc.maxFrames = 0;

#if !NOVACORE_HAS_SDL3
    desc.maxFrames = 5;
#endif

    novacore::core::Application app(desc);
    app.run(game);
    return 0;
}







