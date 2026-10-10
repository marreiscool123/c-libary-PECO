#pragma once

#include <cmath>
#include <cstddef>

namespace peco {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr Vec3() = default;
    constexpr Vec3(float xValue, float yValue, float zValue)
        : x(xValue), y(yValue), z(zValue) {}

    constexpr Vec3 operator+(const Vec3& rhs) const
    {
        return {x + rhs.x, y + rhs.y, z + rhs.z};
    }
    constexpr Vec3 operator-(const Vec3& rhs) const
    {
        return {x - rhs.x, y - rhs.y, z - rhs.z};
    }
    constexpr Vec3 operator-() const { return {-x, -y, -z}; }
    constexpr Vec3 operator*(float scalar) const
    {
        return {x * scalar, y * scalar, z * scalar};
    }
    constexpr Vec3 operator/(float scalar) const
    {
        return {x / scalar, y / scalar, z / scalar};
    }
    constexpr Vec3& operator+=(const Vec3& rhs)
    {
        x += rhs.x; y += rhs.y; z += rhs.z; return *this;
    }
    constexpr Vec3& operator-=(const Vec3& rhs)
    {
        x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this;
    }
    constexpr Vec3& operator*=(float scalar)
    {
        x *= scalar; y *= scalar; z *= scalar; return *this;
    }
    constexpr Vec3& operator/=(float scalar)
    {
        x /= scalar; y /= scalar; z /= scalar; return *this;
    }
    constexpr bool operator==(const Vec3& rhs) const
    {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }
    constexpr bool operator!=(const Vec3& rhs) const { return !(*this == rhs); }
    constexpr Vec3 hadamard(const Vec3& rhs) const { return {x * rhs.x, y * rhs.y, z * rhs.z}; }
    constexpr float dot(const Vec3& rhs) const
    {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }
    constexpr Vec3 cross(const Vec3& rhs) const
    {
        return {y * rhs.z - z * rhs.y,
                z * rhs.x - x * rhs.z,
                x * rhs.y - y * rhs.x};
    }
    constexpr float lengthSquared() const { return dot(*this); }
    float length() const { return std::sqrt(dot(*this)); }
    Vec3 normalized() const
    {
        const float magnitude = length();
        return magnitude > 0.0f ? *this / magnitude : Vec3{};
    }
    Vec3 normalizedOr(const Vec3& fallback) const
    {
        const float magnitude = length();
        return magnitude > 0.0f ? *this / magnitude : fallback;
    }
    constexpr float componentMin() const
    {
        return x < y ? (x < z ? x : z) : (y < z ? y : z);
    }
    constexpr float componentMax() const
    {
        return x > y ? (x > z ? x : z) : (y > z ? y : z);
    }
    constexpr Vec3 abs() const
    {
        return {x < 0.0f ? -x : x, y < 0.0f ? -y : y, z < 0.0f ? -z : z};
    }
    constexpr Vec3 clamp(const Vec3& minimum, const Vec3& maximum) const
    {
        return {
            x < minimum.x ? minimum.x : (x > maximum.x ? maximum.x : x),
            y < minimum.y ? minimum.y : (y > maximum.y ? maximum.y : y),
            z < minimum.z ? minimum.z : (z > maximum.z ? maximum.z : z)
        };
    }
    bool isFinite() const
    {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
    }
    static constexpr Vec3 zero() { return {0.0f, 0.0f, 0.0f}; }
    static constexpr Vec3 one() { return {1.0f, 1.0f, 1.0f}; }
    static constexpr Vec3 unitX() { return {1.0f, 0.0f, 0.0f}; }
    static constexpr Vec3 unitY() { return {0.0f, 1.0f, 0.0f}; }
    static constexpr Vec3 unitZ() { return {0.0f, 0.0f, 1.0f}; }
    static constexpr Vec3 min(const Vec3& lhs, const Vec3& rhs)
    {
        return {lhs.x < rhs.x ? lhs.x : rhs.x,
                lhs.y < rhs.y ? lhs.y : rhs.y,
                lhs.z < rhs.z ? lhs.z : rhs.z};
    }
    static constexpr Vec3 max(const Vec3& lhs, const Vec3& rhs)
    {
        return {lhs.x > rhs.x ? lhs.x : rhs.x,
                lhs.y > rhs.y ? lhs.y : rhs.y,
                lhs.z > rhs.z ? lhs.z : rhs.z};
    }
    static constexpr float distanceSquared(const Vec3& lhs, const Vec3& rhs)
    {
        return (lhs - rhs).lengthSquared();
    }
    static float distance(const Vec3& lhs, const Vec3& rhs)
    {
        return std::sqrt(distanceSquared(lhs, rhs));
    }
    static constexpr Vec3 lerp(const Vec3& lhs, const Vec3& rhs, float amount)
    {
        return lhs + (rhs - lhs) * amount;
    }
    static Vec3 moveTowards(const Vec3& current, const Vec3& target, float maxDistance)
    {
        maxDistance = std::fmax(0.0f, maxDistance);
        const Vec3 delta = target - current;
        const float distanceToTarget = delta.length();
        if (distanceToTarget <= maxDistance || distanceToTarget == 0.0f) {
            return target;
        }
        return current + delta / distanceToTarget * maxDistance;
    }
    static Vec3 reflect(const Vec3& direction, const Vec3& normal)
    {
        return direction - normal * (2.0f * direction.dot(normal));
    }
    static Vec3 project(const Vec3& value, const Vec3& onto)
    {
        const float denominator = onto.lengthSquared();
        return denominator > 0.0f ? onto * (value.dot(onto) / denominator) : Vec3{};
    }
};

constexpr Vec3 operator*(float scalar, const Vec3& value) { return value * scalar; }

} // namespace peco
