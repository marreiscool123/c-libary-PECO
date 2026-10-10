#pragma once

#include <array>
#include <limits>

#include <peco\vector3.hpp>

namespace peco {

struct Bounds3 {
    Vec3 minimum{};
    Vec3 maximum{};

    constexpr Bounds3() = default;
    constexpr Bounds3(const Vec3& minPoint, const Vec3& maxPoint)
        : minimum(minPoint), maximum(maxPoint) {}

    static Bounds3 empty()
    {
        const float infinity = std::numeric_limits<float>::infinity();
        return {{infinity, infinity, infinity}, {-infinity, -infinity, -infinity}};
    }
    static constexpr Bounds3 fromCenterSize(const Vec3& center, const Vec3& size)
    {
        const Vec3 half = size * 0.5f;
        return {center - half, center + half};
    }
    constexpr Vec3 center() const { return (minimum + maximum) * 0.5f; }
    constexpr Vec3 size() const { return maximum - minimum; }
    constexpr Vec3 extents() const { return size() * 0.5f; }
    constexpr float volume() const
    {
        const Vec3 dimensions = size();
        return dimensions.x * dimensions.y * dimensions.z;
    }
    constexpr bool isEmpty() const
    {
        return minimum.x > maximum.x || minimum.y > maximum.y || minimum.z > maximum.z;
    }
    constexpr bool contains(const Vec3& point) const
    {
        return !isEmpty() && point.x >= minimum.x && point.x <= maximum.x
            && point.y >= minimum.y && point.y <= maximum.y
            && point.z >= minimum.z && point.z <= maximum.z;
    }
    constexpr bool contains(const Bounds3& other) const
    {
        return !isEmpty() && !other.isEmpty()
            && contains(other.minimum) && contains(other.maximum);
    }
    constexpr bool intersects(const Bounds3& other) const
    {
        return !isEmpty() && !other.isEmpty()
            && minimum.x <= other.maximum.x && maximum.x >= other.minimum.x
            && minimum.y <= other.maximum.y && maximum.y >= other.minimum.y
            && minimum.z <= other.maximum.z && maximum.z >= other.minimum.z;
    }
    constexpr Vec3 closestPoint(const Vec3& point) const
    {
        return point.clamp(minimum, maximum);
    }
    constexpr Bounds3 expanded(const Vec3& point) const
    {
        if (isEmpty()) {
            return {point, point};
        }
        return {Vec3::min(minimum, point), Vec3::max(maximum, point)};
    }
    constexpr Bounds3 united(const Bounds3& other) const
    {
        if (isEmpty()) {
            return other;
        }
        if (other.isEmpty()) {
            return *this;
        }
        return {Vec3::min(minimum, other.minimum), Vec3::max(maximum, other.maximum)};
    }
    constexpr Bounds3 intersection(const Bounds3& other) const
    {
        const Vec3 minPoint = Vec3::max(minimum, other.minimum);
        const Vec3 maxPoint = Vec3::min(maximum, other.maximum);
        return maxPoint.x < minPoint.x || maxPoint.y < minPoint.y
            || maxPoint.z < minPoint.z
            ? Bounds3::empty() : Bounds3{minPoint, maxPoint};
    }
    constexpr Bounds3 translated(const Vec3& offset) const
    {
        return {minimum + offset, maximum + offset};
    }
    constexpr Bounds3 inflated(const Vec3& amount) const
    {
        return {minimum - amount, maximum + amount};
    }
    std::array<Vec3, 8> corners() const
    {
        return {{
            {minimum.x, minimum.y, minimum.z},
            {maximum.x, minimum.y, minimum.z},
            {minimum.x, maximum.y, minimum.z},
            {maximum.x, maximum.y, minimum.z},
            {minimum.x, minimum.y, maximum.z},
            {maximum.x, minimum.y, maximum.z},
            {minimum.x, maximum.y, maximum.z},
            {maximum.x, maximum.y, maximum.z}
        }};
    }
};

} // namespace peco
