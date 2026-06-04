#include "fps/core/Application.hpp"
#include "fps/core/Log.hpp"
#include "fps/ecs/World.hpp"
#include "fps/platform/Input.hpp"
#include "fps/platform/Window.hpp"
#include "fps/render/Renderer.hpp"

#include <array>

namespace {

class GameApp final : public riftline::core::IApplicationDelegate {
public:
    void onStartup() override {
        riftline::platform::WindowDesc windowDesc{};
        windowDesc.title = "Project Riftline - M1 Thin Spine";
        windowDesc.width = 1280;
        windowDesc.height = 720;
        windowDesc.preferVulkan = true;

        window_.create(windowDesc);

        riftline::render::RendererCreateInfo rendererInfo{};
        renderer_.create(window_, rendererInfo);

        const auto camera = world_.createEntity();
        world_.addComponent(camera, riftline::ecs::NameComponent{"main_camera"});
        world_.addComponent(camera, riftline::ecs::TransformComponent{});
        world_.addComponent(camera, riftline::ecs::CameraComponent{});

        riftline::core::logInfo("game", "Sandbox camera entity created");
    }

    void onShutdown() override {
        renderer_.shutdown();
        window_.shutdown();
    }

    void onFixedTick(const riftline::core::FrameContext& context) override {
        (void)context;
    }

    void onFrame(const riftline::core::FrameContext& context) override {
        (void)context;
        window_.pollEvents(input_);

        riftline::render::RenderFrameInfo frameInfo{};
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
    riftline::platform::InputSystem input_;
    riftline::platform::Window window_;
    riftline::render::Renderer renderer_;
    riftline::ecs::World world_;
};

} // namespace

int main() {
    GameApp game;

    riftline::core::ApplicationDesc desc{};
    desc.name = "riftline_game";
    desc.fixedTickHz = 60.0;
    desc.maxFrames = 0;

#if !RIFTLINE_HAS_SDL3
    desc.maxFrames = 5;
#endif

    riftline::core::Application app(desc);
    app.run(game);
    return 0;
}

