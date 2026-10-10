#pragma once

#include <algorithm>

#include <peco\color.hpp>
#include <peco\vector3.hpp>

namespace peco {

enum class LightType {
    Directional,
    Point,
    Spot
};

struct Light {
    LightType type = LightType::Directional;
    Vec3 position{0.0f, 3.0f, 0.0f};
    Vec3 direction{0.0f, -1.0f, 0.0f};
    Color color = Color::white();
    float intensity = 1.0f;
    float range = 10.0f;
    float innerConeRadians = 0.35f;
    float outerConeRadians = 0.6f;
    bool enabled = true;

    Vec3 normalizedDirection() const { return direction.normalized(); }
    float attenuation(const Vec3& point) const
    {
        if (type == LightType::Directional) {
            return enabled ? std::fmax(0.0f, intensity) : 0.0f;
        }
        const float distance = Vec3::distance(position, point);
        if (range <= 0.0f || distance >= range) {
            return 0.0f;
        }
        const float normalizedDistance = distance / range;
        float falloff = (1.0f - normalizedDistance * normalizedDistance);
        falloff *= falloff;
        if (type == LightType::Spot) {
            const Vec3 fromLight = (point - position).normalized();
            const float cosine = fromLight.dot(normalizedDirection());
            const float inner = std::cos(innerConeRadians);
            const float outer = std::cos(outerConeRadians);
            const float cone = outer == inner ? (cosine >= inner ? 1.0f : 0.0f)
                : std::clamp((cosine - outer) / (inner - outer), 0.0f, 1.0f);
            falloff *= cone;
        }
        return enabled ? std::fmax(0.0f, intensity) * falloff : 0.0f;
    }
    Vec3 directionAt(const Vec3& point) const
    {
        return type == LightType::Directional
            ? -normalizedDirection()
            : (position - point).normalized();
    }
    Vec3 radianceAt(const Vec3& point) const
    {
        const float energy = attenuation(point);
        return {
            static_cast<float>(color.r) / 255.0f * energy,
            static_cast<float>(color.g) / 255.0f * energy,
            static_cast<float>(color.b) / 255.0f * energy
        };
    }
};

struct AmbientLight {
    Color color{36, 42, 56};
    float intensity = 0.25f;
    Vec3 radiance() const
    {
        return {
            static_cast<float>(color.r) / 255.0f * intensity,
            static_cast<float>(color.g) / 255.0f * intensity,
            static_cast<float>(color.b) / 255.0f * intensity
        };
    }
};

inline float lambertDiffuse(const Vec3& normal, const Vec3& lightDirection)
{
    return std::fmax(0.0f, normal.normalized().dot(lightDirection.normalized()));
}

inline float blinnPhongSpecular(const Vec3& normal, const Vec3& lightDirection,
                                const Vec3& viewDirection, float shininess)
{
    const Vec3 halfVector = (lightDirection + viewDirection).normalized();
    const float cosine = std::fmax(0.0f, normal.normalized().dot(halfVector));
    return std::pow(cosine, std::fmax(1.0f, shininess));
}

} // namespace peco
