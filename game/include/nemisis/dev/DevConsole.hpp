#pragma once

#include "nemisis/ui/GameMenu.hpp"

#include <array>

namespace nemisis::net { struct MatchFrame; }
namespace nemisis::dev {
struct DevTargetRangeState;

class DevConsole final {
public:
    [[nodiscard]] bool update(const novacore::platform::InputActionMap& actions, ui::MenuPointerState pointer,
                              bool mainMenu);
    [[nodiscard]] bool isOpen() const { return open_; }
    void apply(novacore::render::RenderFrameInfo& frame, const DevTargetRangeState* targets,
               const net::MatchFrame* privateMatch, unsigned localSlot) const;
    void draw(novacore::render::RenderFrameInfo& frame, const novacore::render::RenderBackendFrameStats& stats,
              bool mainMenu, bool canInspectActors) const;

private:
    enum Row { HitZones, Textures, ShadowMap, Contact, Reset, Count };
    std::array<int, Count> values_{};
    int selected_ = 0;
    bool open_ = false;
    void change(int direction);
};
}
