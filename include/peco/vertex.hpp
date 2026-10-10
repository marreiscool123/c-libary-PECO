#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <peco\color.hpp>
#include <peco\vector2.hpp>
#include <peco\vector3.hpp>

namespace peco {

struct Vertex2D {
    Vec2 position{};
    Vec2 textureCoordinate{};
    Color color = Color::white();

    constexpr Vertex2D() = default;
    constexpr explicit Vertex2D(const Vec2& positionValue)
        : position(positionValue) {}
    constexpr Vertex2D(const Vec2& positionValue, const Vec2& uv)
        : position(positionValue), textureCoordinate(uv) {}
    constexpr Vertex2D(const Vec2& positionValue, const Vec2& uv, Color vertexColor)
        : position(positionValue), textureCoordinate(uv), color(vertexColor) {}
};

struct Vertex3D {
    Vec3 position{};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    Vec2 textureCoordinate{};
    Color color = Color::white();

    constexpr Vertex3D() = default;
    constexpr explicit Vertex3D(const Vec3& positionValue)
        : position(positionValue) {}
    constexpr Vertex3D(const Vec3& positionValue, const Vec3& normalValue)
        : position(positionValue), normal(normalValue) {}
    constexpr Vertex3D(const Vec3& positionValue, const Vec3& normalValue,
                       const Vec2& uv)
        : position(positionValue), normal(normalValue), textureCoordinate(uv) {}
    constexpr Vertex3D(const Vec3& positionValue, const Vec3& normalValue,
                       const Vec2& uv, Color vertexColor)
        : position(positionValue), normal(normalValue),
          textureCoordinate(uv), color(vertexColor) {}
};

struct Vertex3DHash {
    std::size_t operator()(const Vertex3D& vertex) const noexcept
    {
        const auto bits = [](float value) {
            std::uint32_t result = 0;
            static_assert(sizeof(result) == sizeof(value), "Unexpected float width");
            std::memcpy(&result, &value, sizeof(result));
            return result;
        };
        std::size_t hash = bits(vertex.position.x);
        const auto combine = [&hash](std::uint32_t value) {
            hash ^= static_cast<std::size_t>(value) + 0x9e3779b9u
                + (hash << 6u) + (hash >> 2u);
        };
        combine(bits(vertex.position.y));
        combine(bits(vertex.position.z));
        combine(bits(vertex.normal.x));
        combine(bits(vertex.normal.y));
        combine(bits(vertex.normal.z));
        combine(bits(vertex.textureCoordinate.x));
        combine(bits(vertex.textureCoordinate.y));
        combine(vertex.color.rgba());
        return hash;
    }
};

inline bool operator==(const Vertex3D& lhs, const Vertex3D& rhs)
{
    return lhs.position == rhs.position && lhs.normal == rhs.normal
        && lhs.textureCoordinate == rhs.textureCoordinate && lhs.color == rhs.color;
}

} // namespace peco
