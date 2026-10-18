#include "nemisis/ui/UiShowcase.hpp"
#include "nemisis/input/InputBindings.hpp"
#include <RmlUi/Core/Input.h>
#include <iostream>
#include <stdexcept>
#include <cmath>
using namespace nemisis;
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
int main() {
    try {
        ui::UiShowcase showcase;
        showcase.initialize();
        novacore::platform::InputSnapshot input;
        auto actions = input::createDefaultActionMap();
        auto tick = [&](int w=1280,int h=720) {
            actions.update(input); return showcase.update(1.0/60,input,actions,w,h);
        };
        auto click = [&](const char* id) {
            const auto r = showcase.bounds(id);
            require(r[2] > 0 && r[3] > 0,"click target has no layout bounds");
            showcase.pointer(static_cast<int>(r[0]+r[2]/2),static_cast<int>(r[1]+r[3]/2),true);
            showcase.pointer(static_cast<int>(r[0]+r[2]/2),static_cast<int>(r[1]+r[3]/2),false);
            tick();
        };
        tick();
        for (auto [w,h] : {std::pair{1280,720},std::pair{1920,1080},std::pair{2560,1440}}) {
            tick(w,h);
            novacore::render::UiDrawData draw;
            showcase.render(draw);
            require(draw.valid(),"invalid native draw geometry");
            require(draw.commands.size()>15 && draw.vertices.size()>100,"document did not produce real UI geometry");
            require(showcase.stats().textureBytes>0,"FreeType font atlas missing");
            for (const char* id : {"launch-range","click-test","motion","intensity"}) {
                auto r=showcase.bounds(id);
                if (!(r[0]>=0 && r[1]>=0 && r[0]+r[2]<=w+1 && r[1]+r[3]<=h+1)) {
                    std::cerr << id << " bounds " << r[0] << ',' << r[1] << ',' << r[2] << ',' << r[3] << " at " << w << 'x' << h << '\n';
                    throw std::runtime_error("core control outside viewport");
                }
            }
            std::cout << w << 'x' << h << ": " << draw.commands.size() << " draws, " << draw.vertices.size()
                << " vertices, " << showcase.stats().textureBytes << " atlas bytes\n";
        }
        tick();
        click("click-test");
        require(showcase.value("click-count").find("1 confirmed")!=std::string::npos,"mouse click failed");
        click("disabled-button");
        require(showcase.value("click-count").find("1 confirmed")!=std::string::npos,"disabled action changed counter");
        click("motion");
        require(showcase.value("motion-value")=="Reduced","toggle did not update native state");
        click("intensity");
        const auto before = std::stof(showcase.value("intensity"));
        showcase.key(Rml::Input::KI_RIGHT,true); showcase.key(Rml::Input::KI_RIGHT,false); tick();
        require(std::stof(showcase.value("intensity"))>before,"keyboard slider adjustment failed");
        click("open-modal");
        require(showcase.focusId()=="close-modal","modal focus missing");
        // Tab stays in modal and cannot activate any background control.
        for (int i=0;i<7;++i) {
            showcase.key(Rml::Input::KI_TAB,true); showcase.key(Rml::Input::KI_TAB,false); tick();
            require(showcase.focusId()=="close-modal" || showcase.focusId()=="confirm-modal","modal focus escaped");
        }
        input.setButton({novacore::platform::InputControlKind::GamepadButton,input::gamepad_buttons::B},true,novacore::platform::InputDeviceKind::Controller);
        tick(); input.beginFrame(); input.setButton({novacore::platform::InputControlKind::GamepadButton,input::gamepad_buttons::B},false,novacore::platform::InputDeviceKind::Controller); tick(); input.beginFrame();
        require(showcase.focusId()=="open-modal","controller B did not restore modal opener focus");
        click("tab-type");
        require(showcase.bounds("metric-wide")[2]>showcase.bounds("metric-narrow")[2]*2,"font advances are not proportional");
        click("tab-input");
        require(showcase.bounds("quality")[2]>100,"tab did not display controls");
        click("quality");
        showcase.key(Rml::Input::KI_DOWN,true); showcase.key(Rml::Input::KI_DOWN,false); tick();
        require(showcase.value("quality-value")=="High","dropdown keyboard selection failed");
        showcase.key(Rml::Input::KI_RETURN,true); showcase.key(Rml::Input::KI_RETURN,false); tick();
        click("tab-components");
        click("click-test");
        const auto counter = showcase.value("click-count");
        input.setButton({novacore::platform::InputControlKind::GamepadButton,input::gamepad_buttons::A},true,novacore::platform::InputDeviceKind::Controller);
        tick(); input.beginFrame(); input.setButton({novacore::platform::InputControlKind::GamepadButton,input::gamepad_buttons::A},false,novacore::platform::InputDeviceKind::Controller); tick(); input.beginFrame();
        require(showcase.value("click-count")!=counter,"controller confirm did not activate focused button");
        novacore::render::UiDrawData steady;
        showcase.render(steady);
        const auto atlas = showcase.stats().textureBytes;
        const auto commands = steady.commands.size();
        for (int frame=0; frame<120; ++frame) { tick(); showcase.render(steady); }
        require(showcase.stats().textureBytes==atlas && steady.commands.size()==commands,"idle UI resources or commands keep growing");
        require(showcase.stats().warningCount==0,"RmlUi reported style/render/font warnings");
        std::cout << "PASS: native layout, atlas, pointer, disabled, toggle, slider, modal, dropdown, controller and bounded idle resources\n";
        return 0;
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
