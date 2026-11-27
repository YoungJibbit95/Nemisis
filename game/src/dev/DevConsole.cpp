#include "nemisis/dev/DevConsole.hpp"

#include "nemisis/dev/DevTargetRange.hpp"
#include "nemisis/input/InputBindings.hpp"
#include "nemisis/net/FacilityMatch.hpp"
#include "nemisis/ui/UiCanvas.hpp"

#include <cmath>
#include <cstdio>
#include <numbers>

namespace nemisis::dev {
namespace {
using namespace novacore;
using ui::UiRect;
constexpr UiRect panel{836, 66, 412, 362};
constexpr UiRect launcher{1030, 660, 212, 38};
constexpr UiRect closeButton{1185, 80, 45, 28};
constexpr float rowTop = 155, rowHeight = 40, rowStep = 44;
constexpr std::array names{"Hit volumes", "Material textures", "Shadow map", "Contact shadows", "Restore scene settings"};
constexpr std::array<std::array<const char*,5>,5> values{{
    {"Off","On"}, {"Scene","On","Off"}, {"Scene","Off","512","1024","2048"}, {"Scene","Off"}, {"Reset"}
}};
constexpr std::array counts{2,3,5,2,1};

bool inside(ui::MenuPointerState p, UiRect r) {
    return p.available && p.x >= r.x && p.x < r.x+r.width && p.y >= r.y && p.y < r.y+r.height;
}
UiRect rowBounds(int row) { return {panel.x+14, rowTop+row*rowStep, panel.width-28, rowHeight}; }

void humanoidLines(render::RenderFrameInfo& frame, math::Vec3 feet, float yawDegrees, float height) {
    const float yaw = yawDegrees * std::numbers::pi_v<float> / 180.0F;
    const float c = std::cos(yaw), s = std::sin(yaw);
    for (const auto& zone : humanoidHitZones) {
        std::array<math::Vec3,8> corners;
        for (unsigned i = 0; i < 8; ++i) {
            const auto p = zone.center + math::Vec3{(i&1) ? zone.half.x : -zone.half.x,
                (i&2) ? zone.half.y : -zone.half.y, (i&4) ? zone.half.z : -zone.half.z};
            corners[i] = feet + math::Vec3{c*p.x+s*p.z, p.y*height, -s*p.x+c*p.z};
        }
        const auto color = zone.type == player::DamageZone::Head ? ui::UiColor{1,.67F,.14F,1}
            : zone.type == player::DamageZone::Body ? ui::UiColor{.12F,1,.92F,1} : ui::UiColor{.49F,.7F,1,1};
        for (unsigned i = 0; i < 8; ++i) {
            for (unsigned edge : {1U,2U,4U}) {
                if (!(i & edge)) frame.worldLines.push_back({corners[i],corners[i|edge],color});
            }
        }
    }
}

void targetLines(render::RenderFrameInfo& frame, const DebugTargetState& target) {
    if (target.eliminated) return;
    if (target.humanoidHitZones) {
        humanoidLines(frame,target.position - math::Vec3{0,1.65F,0},0,1);
        return;
    }
    constexpr unsigned segments = 24;
    for (unsigned axis = 0; axis < 3; ++axis) {
        const auto point = [&](unsigned segment) {
            const float angle = segment * (2 * std::numbers::pi_v<float> / segments);
            const float a = target.radiusMeters*std::cos(angle), b = target.radiusMeters*std::sin(angle);
            return target.position + (axis == 0 ? math::Vec3{0,a,b} : axis == 1 ? math::Vec3{a,0,b} : math::Vec3{a,b,0});
        };
        for (unsigned i = 0; i < segments; ++i) {
            frame.worldLines.push_back({point(i),point(i+1),{.12F,1,.92F,1}});
        }
    }
}
}

void DevConsole::change(int direction) {
    if (selected_ == Reset) { values_.fill(0); return; }
    const int count = counts[selected_];
    values_[selected_] = (values_[selected_] + count + direction) % count;
}

bool DevConsole::update(const novacore::platform::InputActionMap& actions, ui::MenuPointerState pointer, bool mainMenu) {
    const auto pressed = [&](std::string_view id) { return actions.stateOrDefault(id).pressed; };
    using namespace input::actions;
    if (pressed(input::actions::DevConsole) || (!open_ && mainMenu && inside(pointer,launcher) && pointer.primaryPressed)) {
        open_ = !open_;
        return true;
    }
    if (!open_) return false;
    if (pressed(MenuBack) || (inside(pointer,closeButton) && pointer.primaryPressed)) {
        open_ = false;
        return true;
    }
    if (pressed(MenuUp)) selected_ = (selected_ + Count - 1) % Count;
    if (pressed(MenuDown)) selected_ = (selected_ + 1) % Count;
    if (pressed(MenuLeft)) change(-1);
    else if (pressed(MenuRight) || pressed(MenuConfirm)) change(1);
    if (pointer.primaryPressed) {
        for (int row = 0; row < Count; ++row) {
            if (inside(pointer,rowBounds(row))) {
                selected_ = row;
                change(1);
                break;
            }
        }
    }
    return true;
}

void DevConsole::apply(novacore::render::RenderFrameInfo& frame, const DevTargetRangeState* targets,
                       const net::MatchFrame* match, unsigned localSlot) const {
    if (values_[Textures]) {
        frame.materialTexturesEnabled = values_[Textures] == 1;
        if (!frame.materialTexturesEnabled) frame.skyEnvironmentReflectionsEnabled = false;
    }
    if (values_[ShadowMap]) {
        frame.directionalShadows.enabled = values_[ShadowMap] != 1;
        if (frame.directionalShadows.enabled) frame.directionalShadows.resolution = 512U << (values_[ShadowMap]-2);
    }
    if (values_[Contact]) frame.lighting.contactShadowsEnabled = false;
    if (!values_[HitZones]) return;
    if (match) {
        for (unsigned i = 0; i < match->rules.teamSize * 2; ++i) {
            const auto& actor = match->players[i];
            if (i == localSlot || actor.health.eliminated) continue;
            humanoidLines(frame,actor.movement.position,actor.view.yawDegrees,hitZoneHeightScale(actor.movement.crouchFraction));
        }
    } else if (targets) {
        for (const auto& lane : targets->lanes) targetLines(frame,lane.target);
    }
}

void DevConsole::draw(novacore::render::RenderFrameInfo& frame, const novacore::render::RenderBackendFrameStats& stats,
                      bool mainMenu, bool canInspectActors) const {
    if (!open_ && !mainMenu) return;
    const auto firstRect = frame.debugRects.size(), firstLine = frame.debugLines.size(), firstText = frame.debugTexts.size();
    ui::UiCanvas canvas;
    canvas.beginFrame();
    if (!open_) {
        canvas.button(launcher,"Dev console","O",false);
    } else {
        canvas.panel(panel,{.fill={.025F,.046F,.06F,.98F},.showAccent=true});
        canvas.text(panel.x+20,panel.y+18,2.0F,{.90F,.96F,.98F,1},"Dev console");
        canvas.button(closeButton,"X","",false);
        canvas.text(panel.x+20,panel.y+54,1.1F,{.63F,.80F,.84F,1},"Up / Down to select    Enter to change");
        for (int row = 0; row < Count; ++row) {
            const char* value = values[row][values_[row]];
            if (row == HitZones && !canInspectActors) value = "Range / private";
            canvas.button(rowBounds(row),names[row],value,row == selected_);
        }
        canvas.text(panel.x+20,panel.y+panel.height-38,1.05F,{.67F,.85F,.88F,1},"Esc closes the console");
    }
    canvas.appendToRenderFrame(frame);
    ui::fitInterfaceCanvas(frame,stats,firstRect,firstLine,firstText);
}

} // namespace nemisis::dev
