#pragma once

#include <cmath>

#include <peco\vector3.hpp>

namespace peco {

struct Quaternion {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;

    constexpr Quaternion() = default;
    constexpr Quaternion(float xValue, float yValue, float zValue, float wValue)
        : x(xValue), y(yValue), z(zValue), w(wValue) {}

    constexpr Quaternion operator+(const Quaternion& rhs) const
    {
        return {x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w};
    }
    constexpr Quaternion operator-(const Quaternion& rhs) const
    {
        return {x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w};
    }
    constexpr Quaternion operator-() const { return {-x, -y, -z, -w}; }
    constexpr Quaternion operator*(float scalar) const
    {
        return {x * scalar, y * scalar, z * scalar, w * scalar};
    }
    constexpr Quaternion operator*(const Quaternion& rhs) const
    {
        return {
            w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
            w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
            w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w,
            w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z
        };
    }
    constexpr float dot(const Quaternion& rhs) const
    {
        return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
    }
    constexpr float lengthSquared() const { return dot(*this); }
    float length() const { return std::sqrt(lengthSquared()); }
    Quaternion normalized() const
    {
        const float magnitude = length();
        return magnitude > 0.0f ? *this * (1.0f / magnitude) : identity();
    }
    Quaternion conjugate() const { return {-x, -y, -z, w}; }
    Quaternion inverse() const
    {
        const float magnitudeSquared = lengthSquared();
        return magnitudeSquared > 0.0f
            ? conjugate() * (1.0f / magnitudeSquared)
            : identity();
    }
    Vec3 rotate(const Vec3& value) const
    {
        const Quaternion vector{value.x, value.y, value.z, 0.0f};
        const Quaternion result = normalized() * vector * normalized().conjugate();
        return {result.x, result.y, result.z};
    }
    Vec3 axis() const
    {
        const Quaternion unit = normalized();
        const float denominator = std::sqrt(1.0f - unit.w * unit.w);
        return denominator < 0.0001f
            ? Vec3::unitY()
            : Vec3{unit.x / denominator, unit.y / denominator, unit.z / denominator};
    }
    float angle() const
    {
        return 2.0f * std::acos(std::fmax(-1.0f, std::fmin(1.0f, normalized().w)));
    }

    static constexpr Quaternion identity() { return {}; }
    static Quaternion axisAngle(const Vec3& axisValue, float radians)
    {
        const Vec3 unit = axisValue.normalized();
        const float halfAngle = radians * 0.5f;
        const float sine = std::sin(halfAngle);
        return Quaternion{unit.x * sine, unit.y * sine, unit.z * sine,
                          std::cos(halfAngle)}.normalized();
    }
    static Quaternion fromEuler(const Vec3& radians)
    {
        const float cx = std::cos(radians.x * 0.5f);
        const float sx = std::sin(radians.x * 0.5f);
        const float cy = std::cos(radians.y * 0.5f);
        const float sy = std::sin(radians.y * 0.5f);
        const float cz = std::cos(radians.z * 0.5f);
        const float sz = std::sin(radians.z * 0.5f);
        return {
            sx * cy * cz - cx * sy * sz,
            cx * sy * cz + sx * cy * sz,
            cx * cy * sz - sx * sy * cz,
            cx * cy * cz + sx * sy * sz
        };
    }
    static Quaternion lookRotation(const Vec3& forward, const Vec3& up)
    {
        const Vec3 f = forward.normalized();
        if (f.lengthSquared() == 0.0f) {
            return identity();
        }
        const Vec3 r = up.cross(f).normalized();
        const Vec3 u = f.cross(r);
        const float trace = r.x + u.y + f.z;
        if (trace > 0.0f) {
            const float s = std::sqrt(trace + 1.0f) * 2.0f;
            return { (u.z - f.y) / s, (f.x - r.z) / s,
                     (r.y - u.x) / s, 0.25f * s };
        }
        if (r.x > u.y && r.x > f.z) {
            const float s = std::sqrt(1.0f + r.x - u.y - f.z) * 2.0f;
            return {0.25f * s, (r.y + u.x) / s, (r.z + f.x) / s,
                    (u.z - f.y) / s};
        }
        if (u.y > f.z) {
            const float s = std::sqrt(1.0f + u.y - r.x - f.z) * 2.0f;
            return {(r.y + u.x) / s, 0.25f * s, (u.z + f.y) / s,
                    (f.x - r.z) / s};
        }
        const float s = std::sqrt(1.0f + f.z - r.x - u.y) * 2.0f;
        return {(r.z + f.x) / s, (u.z + f.y) / s, 0.25f * s,
                (r.y - u.x) / s};
    }
    static Quaternion slerp(Quaternion lhs, Quaternion rhs, float amount)
    {
        float cosine = lhs.dot(rhs);
        if (cosine < 0.0f) {
            rhs = -rhs;
            cosine = -cosine;
        }
        if (cosine > 0.9995f) {
            return (lhs + (rhs - lhs) * amount).normalized();
        }
        cosine = std::fmax(-1.0f, std::fmin(1.0f, cosine));
        const float angleValue = std::acos(cosine);
        const float sine = std::sin(angleValue);
        const float leftWeight = std::sin((1.0f - amount) * angleValue) / sine;
        const float rightWeight = std::sin(amount * angleValue) / sine;
        return (lhs * leftWeight + rhs * rightWeight).normalized();
    }
};

constexpr Quaternion operator*(float scalar, const Quaternion& value)
{
    return value * scalar;
}

} // namespace peco
