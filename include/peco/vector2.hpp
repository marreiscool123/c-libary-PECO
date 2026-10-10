#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace peco {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    constexpr Vec2() = default;
    constexpr Vec2(float xValue, float yValue) : x(xValue), y(yValue) {}

    constexpr Vec2 operator+(const Vec2& rhs) const { return {x + rhs.x, y + rhs.y}; }
    constexpr Vec2 operator-(const Vec2& rhs) const { return {x - rhs.x, y - rhs.y}; }
    constexpr Vec2 operator*(float scalar) const { return {x * scalar, y * scalar}; }
    constexpr Vec2 operator/(float scalar) const { return {x / scalar, y / scalar}; }
    constexpr Vec2 operator-() const { return {-x, -y}; }
    constexpr Vec2& operator+=(const Vec2& rhs) { x += rhs.x; y += rhs.y; return *this; }
    constexpr Vec2& operator-=(const Vec2& rhs) { x -= rhs.x; y -= rhs.y; return *this; }
    constexpr Vec2& operator*=(float scalar) { x *= scalar; y *= scalar; return *this; }
    constexpr Vec2& operator/=(float scalar) { x /= scalar; y /= scalar; return *this; }
    constexpr bool operator==(const Vec2& rhs) const { return x == rhs.x && y == rhs.y; }
    constexpr bool operator!=(const Vec2& rhs) const { return !(*this == rhs); }
    constexpr Vec2 hadamard(const Vec2& rhs) const { return {x * rhs.x, y * rhs.y}; }
    constexpr float dot(const Vec2& rhs) const { return x * rhs.x + y * rhs.y; }
    constexpr float cross(const Vec2& rhs) const { return x * rhs.y - y * rhs.x; }
    constexpr float lengthSquared() const { return dot(*this); }
    float length() const { return std::sqrt(dot(*this)); }
    constexpr Vec2 perpendicular() const { return {-y, x}; }
    constexpr Vec2 perpendicularClockwise() const { return {y, -x}; }
    Vec2 rotated(float radians) const
    {
        const float cosine = std::cos(radians);
        const float sine = std::sin(radians);
        return {x * cosine - y * sine, x * sine + y * cosine};
    }
    Vec2 normalized() const
    {
        const float magnitude = length();
        return magnitude > 0.0f ? *this / magnitude : Vec2{};
    }
    Vec2 normalizedOr(const Vec2& fallback) const
    {
        const float magnitude = length();
        return magnitude > 0.0f ? *this / magnitude : fallback;
    }
    constexpr Vec2 clamp(const Vec2& minimum, const Vec2& maximum) const
    {
        return {x < minimum.x ? minimum.x : (x > maximum.x ? maximum.x : x),
                y < minimum.y ? minimum.y : (y > maximum.y ? maximum.y : y)};
    }
    constexpr float componentMin() const { return x < y ? x : y; }
    constexpr float componentMax() const { return x > y ? x : y; }
    bool isFinite() const { return std::isfinite(x) && std::isfinite(y); }
    static constexpr Vec2 zero() { return {0.0f, 0.0f}; }
    static constexpr Vec2 one() { return {1.0f, 1.0f}; }
    static constexpr Vec2 unitX() { return {1.0f, 0.0f}; }
    static constexpr Vec2 unitY() { return {0.0f, 1.0f}; }
    static Vec2 fromAngle(float radians) { return {std::cos(radians), std::sin(radians)}; }
    static constexpr Vec2 min(const Vec2& lhs, const Vec2& rhs)
    {
        return {lhs.x < rhs.x ? lhs.x : rhs.x, lhs.y < rhs.y ? lhs.y : rhs.y};
    }
    static constexpr Vec2 max(const Vec2& lhs, const Vec2& rhs)
    {
        return {lhs.x > rhs.x ? lhs.x : rhs.x, lhs.y > rhs.y ? lhs.y : rhs.y};
    }
    static constexpr float distanceSquared(const Vec2& lhs, const Vec2& rhs)
    {
        return (lhs - rhs).lengthSquared();
    }
    static float distance(const Vec2& lhs, const Vec2& rhs)
    {
        return std::sqrt(distanceSquared(lhs, rhs));
    }
    static constexpr Vec2 lerp(const Vec2& lhs, const Vec2& rhs, float amount)
    {
        return lhs + (rhs - lhs) * amount;
    }
    static Vec2 moveTowards(const Vec2& current, const Vec2& target, float maxDistance)
    {
        maxDistance = std::max(0.0f, maxDistance);
        const Vec2 delta = target - current;
        const float distanceToTarget = delta.length();
        if (distanceToTarget <= maxDistance || distanceToTarget == 0.0f) {
            return target;
        }
        return current + delta / distanceToTarget * maxDistance;
    }
    static Vec2 reflect(const Vec2& direction, const Vec2& normal)
    {
        return direction - normal * (2.0f * direction.dot(normal));
    }
    static Vec2 project(const Vec2& value, const Vec2& onto)
    {
        const float denominator = onto.lengthSquared();
        return denominator > 0.0f ? onto * (value.dot(onto) / denominator) : Vec2{};
    }
};

constexpr Vec2 operator*(float scalar, const Vec2& value) { return value * scalar; }

} // namespace peco
