#pragma once
#include "novacore/render/UiDrawData.hpp"
#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/Matrix4.h>
#include <unordered_map>

namespace nemisis::ui {
class RmlRenderBridge final : public Rml::RenderInterface {
public:
    void begin(novacore::render::UiDrawData& output, int width, int height);
    void end();
    Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
    void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) override;
    void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;
    Rml::TextureHandle LoadTexture(Rml::Vector2i&, const Rml::String&) override;
    Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> data, Rml::Vector2i dimensions) override;
    void ReleaseTexture(Rml::TextureHandle texture) override;
    void EnableScissorRegion(bool enable) override;
    void SetScissorRegion(Rml::Rectanglei region) override;
    void SetTransform(const Rml::Matrix4f* transform) override;
    [[nodiscard]] std::size_t textureBytes() const;
private:
    struct Geometry { std::vector<Rml::Vertex> vertices; std::vector<int> indices; };
    std::unordered_map<Rml::CompiledGeometryHandle, Geometry> geometry_;
    std::unordered_map<Rml::TextureHandle, std::shared_ptr<novacore::render::UiTexture>> textures_;
    std::uintptr_t nextHandle_ = 1;
    novacore::render::UiDrawData* frame_ = nullptr;
    Rml::Matrix4f transform_ = Rml::Matrix4f::Identity();
    std::array<float,4> clip_{};
    bool scissor_ = false;
};
}
