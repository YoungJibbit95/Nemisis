#include "RmlRenderBridge.hpp"
#include <RmlUi/Core/Log.h>
#include <cmath>

namespace nemisis::ui {
void RmlRenderBridge::begin(novacore::render::UiDrawData& output, int width, int height) {
    frame_ = &output;
    output.vertices.clear(); output.indices.clear(); output.commands.clear();
    output.width = static_cast<float>(width); output.height = static_cast<float>(height);
    scissor_ = false;
    transform_ = Rml::Matrix4f::Identity();
}
void RmlRenderBridge::end() { frame_ = nullptr; }
Rml::CompiledGeometryHandle RmlRenderBridge::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) {
    const auto handle = nextHandle_++;
    geometry_.emplace(handle, Geometry{{vertices.begin(),vertices.end()},{indices.begin(),indices.end()}});
    return handle;
}
void RmlRenderBridge::RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture) {
    if (!frame_) return;
    const auto found = geometry_.find(handle);
    if (found == geometry_.end()) return;
    const auto& data = found->second;
    novacore::render::UiDrawCommand draw;
    draw.firstIndex = static_cast<std::uint32_t>(frame_->indices.size());
    draw.indexCount = static_cast<std::uint32_t>(data.indices.size());
    draw.clip = scissor_ ? clip_ : std::array<float,4>{0,0,frame_->width,frame_->height};
    if (texture) {
        const auto image = textures_.find(texture);
        if (image == textures_.end()) return;
        draw.texture = image->second;
    }
    const auto base = static_cast<std::uint32_t>(frame_->vertices.size());
    for (const auto& vertex : data.vertices) {
        const auto p = transform_ * Rml::Vector4f(vertex.position.x + translation.x, vertex.position.y + translation.y, 0, 1);
        const float w = std::abs(p.w) > 0.00001F ? p.w : 1;
        const auto c = vertex.colour;
        frame_->vertices.push_back({{p.x/w,p.y/w},{vertex.tex_coord.x,vertex.tex_coord.y},{c.red,c.green,c.blue,c.alpha}});
    }
    for (auto index : data.indices) frame_->indices.push_back(base + static_cast<std::uint32_t>(index));
    frame_->commands.push_back(std::move(draw));
}
void RmlRenderBridge::ReleaseGeometry(Rml::CompiledGeometryHandle handle) { geometry_.erase(handle); }
Rml::TextureHandle RmlRenderBridge::GenerateTexture(Rml::Span<const Rml::byte> data, Rml::Vector2i size) {
    if (size.x <= 0 || size.y <= 0 || size.x > 4096 || size.y > 4096) return 0;
    auto image = std::make_shared<novacore::render::UiTexture>();
    image->width = static_cast<std::uint32_t>(size.x); image->height = static_cast<std::uint32_t>(size.y);
    image->rgba.assign(data.begin(), data.end());
    if (!image->valid()) return 0;
    const auto handle = nextHandle_++;
    textures_.emplace(handle,std::move(image)); return handle;
}
Rml::TextureHandle RmlRenderBridge::LoadTexture(Rml::Vector2i&, const Rml::String& source) {
    Rml::Log::Message(Rml::Log::LT_WARNING,"UI image loading is not available in this spike: %s",source.c_str());
    return 0;
}
void RmlRenderBridge::ReleaseTexture(Rml::TextureHandle texture) { textures_.erase(texture); }
void RmlRenderBridge::EnableScissorRegion(bool enable) { scissor_ = enable; }
void RmlRenderBridge::SetScissorRegion(Rml::Rectanglei region) {
    clip_ = {static_cast<float>(region.Left()),static_cast<float>(region.Top()),
        static_cast<float>(region.Width()),static_cast<float>(region.Height())};
}
void RmlRenderBridge::SetTransform(const Rml::Matrix4f* transform) { transform_ = transform ? *transform : Rml::Matrix4f::Identity(); }
std::size_t RmlRenderBridge::textureBytes() const {
    std::size_t total = 0; for (const auto& [id,texture] : textures_) { (void)id; total += texture->rgba.size(); } return total;
}
}
