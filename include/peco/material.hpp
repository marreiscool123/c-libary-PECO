#pragma once

#include <algorithm>
#include <string>

#include <peco\color.hpp>
#include <peco\vector3.hpp>

namespace peco {

enum class ShadingModel {
    Unlit,
    Lambert,
    BlinnPhong
};

struct Material {
    std::string name;
    Color baseColor{220, 220, 230};
    Color emissiveColor{0, 0, 0, 255};
    ShadingModel shading = ShadingModel::Lambert;
    float metallic = 0.0f;
    float roughness = 0.7f;
    float opacity = 1.0f;
    float shininess = 32.0f;
    bool doubleSided = false;

    void setMetallic(float value)
    {
        metallic = std::clamp(value, 0.0f, 1.0f);
    }
    void setRoughness(float value)
    {
        roughness = std::clamp(value, 0.04f, 1.0f);
    }
    void setOpacity(float value)
    {
        opacity = std::clamp(value, 0.0f, 1.0f);
        baseColor.a = static_cast<std::uint8_t>(opacity * 255.0f);
    }
    bool isTransparent() const { return opacity < 1.0f || baseColor.a < 255; }
    bool isOpaque() const { return !isTransparent(); }

    static Material unlit(Color color)
    {
        Material material;
        material.baseColor = color;
        material.shading = ShadingModel::Unlit;
        return material;
    }
    static Material matte(Color color)
    {
        Material material;
        material.baseColor = color;
        material.roughness = 0.95f;
        material.shininess = 4.0f;
        return material;
    }
    static Material glossy(Color color, float gloss = 0.8f)
    {
        Material material;
        material.baseColor = color;
        material.roughness = 1.0f - std::clamp(gloss, 0.0f, 1.0f);
        material.shininess = 8.0f + 120.0f * gloss;
        return material;
    }
};

struct SurfaceSample {
    Vec3 position{};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    Vec3 viewDirection{0.0f, 0.0f, 1.0f};
    Color albedo = Color::white();
    float metallic = 0.0f;
    float roughness = 0.7f;
};

} // namespace peco
