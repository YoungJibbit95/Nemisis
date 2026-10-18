#include "nemisis/ui/UiShowcase.hpp"
#include "nemisis/input/InputBindings.hpp"
#include "RmlRenderBridge.hpp"
#include "novacore/core/Log.hpp"
#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControl.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>

namespace nemisis::ui {
namespace {
using Clock = std::chrono::steady_clock;
double ms(Clock::time_point start) { return std::chrono::duration<double,std::milli>(Clock::now()-start).count(); }
struct UiClock final : Rml::SystemInterface {
    double elapsed = 0;
    std::size_t warningCount = 0;
    double GetElapsedTime() override { return elapsed; }
    bool LogMessage(Rml::Log::Type type, const Rml::String& message) override {
        if (type <= Rml::Log::LT_WARNING) { ++warningCount; novacore::core::logWarning("rmlui",message); }
        return true;
    }
};
}
struct UiShowcase::Impl final : Rml::EventListener {
    // System and renderer must outlive Rml::Shutdown().
    Rml::ElementInstancerGeneric<Rml::ElementFormControlInput> buttons;
    UiClock clock;
    RmlRenderBridge bridge;
    Rml::Context* context = nullptr;
    Rml::ElementDocument* document = nullptr;
    Rml::ElementDocument* modal = nullptr;
    ShowcaseAction pending = ShowcaseAction::None;
    UiShowcaseStats stats;
    int width = 1280, height = 720;
    int clicks = 0;
    float toastRemaining = 0;
    bool initialized = false;
    bool modalOpen = false;
    bool reduced = false;
    Rml::Vector2f lastPointer{-1,-1};
    std::array<float,4> repeat{};
    std::array<bool,4> held{};
    ~Impl() {
        if (initialized) Rml::Shutdown();
        Rml::SetRenderInterface(nullptr);
        Rml::SetSystemInterface(nullptr);
    }
    Rml::Element* element(const std::string& id) const { return document->GetElementById(id); }
    void text(const std::string& id, const std::string& value) { if (auto* e = element(id)) e->SetInnerRML(value); }
    void tap(Rml::Input::KeyIdentifier key, int mods = 0) {
        context->ProcessKeyDown(key,mods); context->ProcessKeyUp(key,mods);
    }
    void closeModal() {
        modal->Hide(); modalOpen = false;
        element("open-modal")->Focus();
    }
    void ProcessEvent(Rml::Event& event) override {
        auto* target = event.GetTargetElement();
        // Clicks can originate in nested label spans.
        if (event.GetId() == Rml::EventId::Click) {
            while (target && target->GetId().empty()) target = target->GetParentNode();
            if (!target) return;
            const auto id = target->GetId();
            if (id == "launch-range") pending = ShowcaseAction::DevRange;
            else if (id == "back-menu") pending = ShowcaseAction::Menu;
            else if (id == "quit") pending = ShowcaseAction::Quit;
            else if (id == "click-test") { ++clicks; text("click-count",std::to_string(clicks) + " confirmed inputs"); }
            else if (id == "open-modal") { modal->Show(Rml::ModalFlag::Modal); modalOpen = true; modal->GetElementById("close-modal")->Focus(); }
            else if (id == "close-modal" || id == "confirm-modal") closeModal();
            else if (id == "toast-button") { toastRemaining = 3; element("toast")->SetClass("visible",true); }
            else if (id == "tab-components" || id == "tab-type" || id == "tab-input") {
                for (const auto* name : {"components","type","input"}) {
                    const bool active = id == std::string("tab-") + name;
                    element(std::string("tab-") + name)->SetClass("selected",active);
                    element(std::string("view-") + name)->SetClass("hidden",!active);
                }
            }
        } else if (event.GetId() == Rml::EventId::Change) {
            const auto id = target->GetId();
            if (id == "motion") {
                reduced = target->HasAttribute("checked");
                document->SetClass("reduced",reduced); modal->SetClass("reduced",reduced);
                text("motion-value",reduced ? "Reduced" : "Standard");
            } else if (id == "intensity") {
                const auto value = event.GetParameter<Rml::String>("value","50");
                text("intensity-value",std::to_string(static_cast<int>(std::stof(value))) + "%");
                const float amount = std::clamp(std::stof(value),0.0F,100.0F)/100.0F;
                element("effect-preview")->SetProperty("opacity",std::to_string(amount));
            } else if (id == "quality") text("quality-value",event.GetParameter<Rml::String>("value","Standard"));
            else if (id == "hitmarkers") text("hitmarker-value",target->HasAttribute("checked") ? "On (preview)" : "Off (preview)");
        }
    }
};
UiShowcase::UiShowcase() : impl_(std::make_unique<Impl>()) {}
UiShowcase::~UiShowcase() {
    if (impl_->initialized) {
        const auto& s = impl_->stats;
        novacore::core::logInfo("ui", "Last CPU frame: update_layout_ms=" + std::to_string(s.updateMilliseconds) +
            " extract_ms=" + std::to_string(s.renderMilliseconds) + " draws=" + std::to_string(s.drawCalls) +
            " vertices=" + std::to_string(s.vertices) + " atlas_bytes=" + std::to_string(s.textureBytes));
    }
}
void UiShowcase::initialize(const std::string& root) {
    auto& p = *impl_;
    if (p.initialized) throw std::runtime_error("UI showcase already initialized");
    Rml::SetSystemInterface(&p.clock); Rml::SetRenderInterface(&p.bridge);
    if (!Rml::Initialise()) throw std::runtime_error("RmlUi initialization failed");
    p.initialized = true;
    Rml::Factory::RegisterElementInstancer("button", &p.buttons);
    if (!Rml::LoadFontFace(root + "/fonts/SpaceGrotesk-Regular.otf") ||
        !Rml::LoadFontFace(root + "/fonts/SpaceGrotesk-Bold.otf")) throw std::runtime_error("Required Space Grotesk fonts are missing");
    p.context = Rml::CreateContext("nemisis-ui",{p.width,p.height});
    if (!p.context) throw std::runtime_error("Cannot create RmlUi context");
    p.document = p.context->LoadDocument(root + "/showcase.rml");
    p.modal = p.context->LoadDocument(root + "/showcase-modal.rml");
    if (!p.document || !p.modal) throw std::runtime_error("Cannot load native UI showcase documents");
    for (auto* doc : {p.document,p.modal}) {
        doc->AddEventListener(Rml::EventId::Click,&p);
        doc->AddEventListener(Rml::EventId::Change,&p);
    }
    p.document->Show();
    p.context->Update();
    p.element("launch-range")->Focus();
}
ShowcaseAction UiShowcase::update(double delta, const novacore::platform::InputSnapshot& snapshot,
    const novacore::platform::InputActionMap& actions, int width, int height) {
    auto& p = *impl_;
    const auto start = Clock::now();
    const auto dt = static_cast<float>(std::isfinite(delta) ? std::clamp(delta,0.0,0.1) : 0.0);
    p.clock.elapsed += dt;
    p.width = std::max(1,width); p.height = std::max(1,height);
    p.context->SetDimensions({p.width,p.height});
    p.context->SetDensityIndependentPixelRatio(std::clamp(static_cast<float>(height)/720.0F,0.75F,2.5F));
    const novacore::platform::InputControl shift{novacore::platform::InputControlKind::KeyboardKey,input::key_codes::LeftShift};
    const int mods = snapshot.isDown(shift) ? Rml::Input::KM_SHIFT : 0;
    if (snapshot.hasPointerPosition()) {
        const auto pos = snapshot.pointerPosition();
        if (pos.x != p.lastPointer.x || pos.y != p.lastPointer.y) {
            p.context->ProcessMouseMove(static_cast<int>(pos.x),static_cast<int>(pos.y),mods);
            p.lastPointer = {pos.x,pos.y};
        }
    }
    const auto primary = actions.stateOrDefault(input::actions::MenuPointerPrimary);
    if (primary.pressed) p.context->ProcessMouseButtonDown(0,mods);
    if (primary.released) p.context->ProcessMouseButtonUp(0,mods);
    const novacore::platform::InputControl tab{novacore::platform::InputControlKind::KeyboardKey,input::key_codes::Tab};
    if (snapshot.wasPressed(tab)) p.tap(Rml::Input::KI_TAB,mods);
    const auto confirm = actions.stateOrDefault(input::actions::MenuConfirm);
    if (confirm.pressed) p.context->ProcessKeyDown(Rml::Input::KI_RETURN,0);
    if (confirm.released) p.context->ProcessKeyUp(Rml::Input::KI_RETURN,0);
    if (actions.stateOrDefault(input::actions::MenuBack).pressed) {
        if (p.modalOpen) p.closeModal(); else p.pending = ShowcaseAction::Menu;
    }
    const std::array<std::string_view,4> names{input::actions::MenuUp,input::actions::MenuDown,input::actions::MenuLeft,input::actions::MenuRight};
    for (std::size_t i = 0; i < names.size(); ++i) {
        auto state = actions.stateOrDefault(names[i]);
        // Existing move actions provide stick navigation without SDL polling or
        // another event consumer. They never reach gameplay while showcase is open.
        if (!state.down && !state.pressed) {
            const std::array<std::string_view,4> stickNames{input::actions::MoveForward,input::actions::MoveBackward,input::actions::MoveLeft,input::actions::MoveRight};
            state = actions.stateOrDefault(stickNames[i]);
            if (state.value < 0.45F) state = {};
        }
        bool fire = state.pressed;
        if (!state.down) p.repeat[i] = 0;
        else if (!p.held[i]) { fire = true; p.repeat[i] = 0.35F; }
        else if ((p.repeat[i] -= dt) <= 0) { fire = true; p.repeat[i] = 0.09F; }
        p.held[i] = state.down;
        if (fire) {
            auto* focus = p.context->GetFocusElement();
            const bool select = focus && focus->GetTagName() == "select";
            if (i < 2 && !select) p.tap(Rml::Input::KI_TAB,i == 0 ? Rml::Input::KM_SHIFT : 0);
            else p.tap(i == 0 ? Rml::Input::KI_UP : i == 1 ? Rml::Input::KI_DOWN : i == 2 ? Rml::Input::KI_LEFT : Rml::Input::KI_RIGHT);
        }
    }
    const float wheel = snapshot.axisValue({novacore::platform::InputControlKind::MouseAxis,2});
    if (wheel != 0) p.context->ProcessMouseWheel(-wheel,mods);
    if (p.toastRemaining > 0) {
        p.toastRemaining = std::max(0.0F,p.toastRemaining-dt);
        if (p.toastRemaining == 0) p.element("toast")->SetClass("visible",false);
    }
    p.context->Update();
    p.stats.updateMilliseconds = ms(start);
    const auto action = p.pending; p.pending = ShowcaseAction::None; return action;
}
void UiShowcase::render(novacore::render::UiDrawData& output) {
    auto& p = *impl_; const auto start = Clock::now();
    p.bridge.begin(output,p.width,p.height); p.context->Render(); p.bridge.end();
    p.stats.renderMilliseconds = ms(start); p.stats.drawCalls = output.commands.size();
    p.stats.vertices = output.vertices.size(); p.stats.textureBytes = p.bridge.textureBytes(); p.stats.warningCount = p.clock.warningCount;
}
UiShowcaseStats UiShowcase::stats() const { return impl_->stats; }
void UiShowcase::key(int code, bool down, int mods) {
    if (down) impl_->context->ProcessKeyDown(static_cast<Rml::Input::KeyIdentifier>(code),mods);
    else impl_->context->ProcessKeyUp(static_cast<Rml::Input::KeyIdentifier>(code),mods);
}
void UiShowcase::pointer(int x, int y, bool down) {
    impl_->context->ProcessMouseMove(x,y,0);
    if (down) impl_->context->ProcessMouseButtonDown(0,0); else impl_->context->ProcessMouseButtonUp(0,0);
}
std::string UiShowcase::value(const std::string& id) const {
    auto* e = impl_->element(id); if (!e) return {};
    if (auto* control = dynamic_cast<Rml::ElementFormControl*>(e)) return control->GetValue();
    return e->GetInnerRML();
}
std::array<float,4> UiShowcase::bounds(const std::string& id) const {
    auto* e = impl_->element(id); if (!e) return {};
    const auto offset = e->GetAbsoluteOffset(Rml::BoxArea::Border);
    const auto size = e->GetBox().GetSize(Rml::BoxArea::Border);
    return {offset.x,offset.y,size.x,size.y};
}
std::string UiShowcase::focusId() const { const auto* e = impl_->context->GetFocusElement(); return e ? e->GetId() : ""; }
}
