#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

#include <peco\vector2.hpp>
#include <peco\vector3.hpp>

namespace peco::math {

constexpr float pi = 3.14159265358979323846f;
constexpr float tau = 6.28318530717958647692f;
constexpr float degreesToRadians = pi / 180.0f;
constexpr float radiansToDegrees = 180.0f / pi;
constexpr float epsilon = 0.000001f;

template<class T>
constexpr T square(T value)
{
    return value * value;
}

template<class T>
constexpr T cube(T value)
{
    return value * value * value;
}

template<class T>
constexpr T sign(T value)
{
    return (T{} < value) - (value < T{});
}

template<class T>
constexpr T abs(T value)
{
    return value < T{} ? -value : value;
}

template<class T>
constexpr T min(T lhs, T rhs)
{
    return lhs < rhs ? lhs : rhs;
}

template<class T>
constexpr T max(T lhs, T rhs)
{
    return lhs > rhs ? lhs : rhs;
}

template<class T>
constexpr T clamp(T value, T minimum, T maximum)
{
    return value < minimum ? minimum : (value > maximum ? maximum : value);
}

template<class T>
constexpr T saturate(T value)
{
    return clamp(value, T{}, T{1});
}

template<class T>
constexpr T lerp(T start, T end, float amount)
{
    return start + (end - start) * amount;
}

template<class T>
constexpr T inverseLerp(T start, T end, T value)
{
    return start == end ? T{} : (value - start) / (end - start);
}

template<class T>
constexpr T remap(T value, T sourceMinimum, T sourceMaximum,
                  T targetMinimum, T targetMaximum)
{
    return lerp(targetMinimum, targetMaximum,
                static_cast<float>(inverseLerp(sourceMinimum, sourceMaximum, value)));
}

template<class T>
constexpr T remapClamped(T value, T sourceMinimum, T sourceMaximum,
                         T targetMinimum, T targetMaximum)
{
    const T normalized = sourceMinimum == sourceMaximum ? T{}
        : (value - sourceMinimum) / (sourceMaximum - sourceMinimum);
    return lerp(targetMinimum, targetMaximum,
                static_cast<float>(saturate(normalized)));
}

template<class T>
constexpr T moveTowards(T current, T target, T maximumDelta)
{
    const T difference = target - current;
    if (abs(difference) <= maximumDelta) {
        return target;
    }
    return current + sign(difference) * maximumDelta;
}

template<class T>
constexpr T wrap(T value, T minimum, T maximum)
{
    const T range = maximum - minimum;
    if (range <= T{}) {
        return minimum;
    }
    T offset = (value - minimum) % range;
    if constexpr (std::is_signed<T>::value) {
        if (offset < T{}) {
            offset += range;
        }
    }
    return minimum + offset;
}

inline float wrapRadians(float radians)
{
    float result = std::fmod(radians, tau);
    return result < 0.0f ? result + tau : result;
}

inline float deltaAngle(float fromRadians, float toRadians)
{
    const float difference = wrapRadians(toRadians - fromRadians);
    return difference > pi ? difference - tau : difference;
}

inline float lerpAngle(float fromRadians, float toRadians, float amount)
{
    return fromRadians + deltaAngle(fromRadians, toRadians) * amount;
}

inline float smoothStep(float edge0, float edge1, float value)
{
    if (edge0 == edge1) {
        return value < edge0 ? 0.0f : 1.0f;
    }
    const float t = saturate((value - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

inline float smootherStep(float edge0, float edge1, float value)
{
    if (edge0 == edge1) {
        return value < edge0 ? 0.0f : 1.0f;
    }
    const float t = saturate((value - edge0) / (edge1 - edge0));
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

inline float damp(float current, float target, float smoothing, float deltaTime)
{
    if (deltaTime <= 0.0f) {
        return current;
    }
    const float amount = 1.0f - std::exp(-std::fmax(0.0f, smoothing) * deltaTime);
    return lerp(current, target, amount);
}

inline Vec2 damp(const Vec2& current, const Vec2& target,
                 float smoothing, float deltaTime)
{
    const float amount = deltaTime <= 0.0f ? 0.0f
        : 1.0f - std::exp(-std::fmax(0.0f, smoothing) * deltaTime);
    return Vec2::lerp(current, target, amount);
}

inline Vec3 damp(const Vec3& current, const Vec3& target,
                 float smoothing, float deltaTime)
{
    const float amount = deltaTime <= 0.0f ? 0.0f
        : 1.0f - std::exp(-std::fmax(0.0f, smoothing) * deltaTime);
    return Vec3::lerp(current, target, amount);
}

template<class T>
constexpr T approach(T value, T target, T maximumStep)
{
    return moveTowards(value, target, maximumStep);
}

inline bool approximately(float lhs, float rhs, float tolerance = epsilon)
{
    return std::fabs(lhs - rhs) <= tolerance
        * std::fmax(1.0f, std::fmax(std::fabs(lhs), std::fabs(rhs)));
}

inline bool approximately(const Vec2& lhs, const Vec2& rhs,
                          float tolerance = epsilon)
{
    return approximately(lhs.x, rhs.x, tolerance)
        && approximately(lhs.y, rhs.y, tolerance);
}

inline bool approximately(const Vec3& lhs, const Vec3& rhs,
                          float tolerance = epsilon)
{
    return approximately(lhs.x, rhs.x, tolerance)
        && approximately(lhs.y, rhs.y, tolerance)
        && approximately(lhs.z, rhs.z, tolerance);
}

inline float quantize(float value, float step)
{
    return step == 0.0f ? value : std::round(value / step) * step;
}

inline float snap(float value, float gridSize)
{
    return quantize(value, gridSize);
}

inline float fraction(float value)
{
    return value - std::floor(value);
}

inline float pingPong(float value, float length)
{
    if (length <= 0.0f) {
        return 0.0f;
    }
    const float repeated = std::fmod(value, 2.0f * length);
    const float positive = repeated < 0.0f ? repeated + 2.0f * length : repeated;
    return length - std::fabs(positive - length);
}

inline float triangleArea(const Vec2& a, const Vec2& b, const Vec2& c)
{
    return std::fabs((b - a).cross(c - a)) * 0.5f;
}

inline float signedTriangleArea(const Vec2& a, const Vec2& b, const Vec2& c)
{
    return (b - a).cross(c - a) * 0.5f;
}

inline Vec3 barycentric(const Vec3& a, const Vec3& b, const Vec3& c,
                        const Vec3& point)
{
    const Vec3 v0 = b - a;
    const Vec3 v1 = c - a;
    const Vec3 v2 = point - a;
    const float d00 = v0.dot(v0);
    const float d01 = v0.dot(v1);
    const float d11 = v1.dot(v1);
    const float d20 = v2.dot(v0);
    const float d21 = v2.dot(v1);
    const float denominator = d00 * d11 - d01 * d01;
    if (std::fabs(denominator) <= epsilon) {
        return {};
    }
    const float v = (d11 * d20 - d01 * d21) / denominator;
    const float w = (d00 * d21 - d01 * d20) / denominator;
    return {1.0f - v - w, v, w};
}

inline bool pointInTriangle(const Vec2& point, const Vec2& a,
                            const Vec2& b, const Vec2& c)
{
    const float area = signedTriangleArea(a, b, c);
    if (std::fabs(area) <= epsilon) {
        return false;
    }
    const float first = (b - a).cross(point - a) / (area * 2.0f);
    const float second = (c - b).cross(point - b) / (area * 2.0f);
    const float third = (a - c).cross(point - c) / (area * 2.0f);
    return first >= -epsilon && second >= -epsilon && third >= -epsilon;
}

inline float cubicBezier(float p0, float p1, float p2, float p3, float t)
{
    const float inverse = 1.0f - t;
    return inverse * inverse * inverse * p0
         + 3.0f * inverse * inverse * t * p1
         + 3.0f * inverse * t * t * p2
         + t * t * t * p3;
}

inline Vec2 cubicBezier(const Vec2& p0, const Vec2& p1,
                        const Vec2& p2, const Vec2& p3, float t)
{
    const float inverse = 1.0f - t;
    return p0 * (inverse * inverse * inverse)
         + p1 * (3.0f * inverse * inverse * t)
         + p2 * (3.0f * inverse * t * t)
         + p3 * (t * t * t);
}

inline Vec3 cubicBezier(const Vec3& p0, const Vec3& p1,
                        const Vec3& p2, const Vec3& p3, float t)
{
    const float inverse = 1.0f - t;
    return p0 * (inverse * inverse * inverse)
         + p1 * (3.0f * inverse * inverse * t)
         + p2 * (3.0f * inverse * t * t)
         + p3 * (t * t * t);
}

inline float cubicBezierDerivative(float p0, float p1, float p2, float p3, float t)
{
    const float inverse = 1.0f - t;
    return 3.0f * inverse * inverse * (p1 - p0)
         + 6.0f * inverse * t * (p2 - p1)
         + 3.0f * t * t * (p3 - p2);
}

inline float catmullRom(float p0, float p1, float p2, float p3, float t)
{
    const float t2 = t * t;
    const float t3 = t2 * t;
    return 0.5f * ((2.0f * p1) + (-p0 + p2) * t
        + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2
        + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

inline Vec2 catmullRom(const Vec2& p0, const Vec2& p1,
                       const Vec2& p2, const Vec2& p3, float t)
{
    return {
        catmullRom(p0.x, p1.x, p2.x, p3.x, t),
        catmullRom(p0.y, p1.y, p2.y, p3.y, t)
    };
}

inline Vec3 catmullRom(const Vec3& p0, const Vec3& p1,
                       const Vec3& p2, const Vec3& p3, float t)
{
    return {
        catmullRom(p0.x, p1.x, p2.x, p3.x, t),
        catmullRom(p0.y, p1.y, p2.y, p3.y, t),
        catmullRom(p0.z, p1.z, p2.z, p3.z, t)
    };
}

inline float dampSpring(float current, float target, float& velocity,
                        float smoothTime, float deltaTime,
                        float maximumSpeed = std::numeric_limits<float>::infinity())
{
    if (deltaTime <= 0.0f) {
        return current;
    }
    smoothTime = std::fmax(0.0001f, smoothTime);
    const float omega = 2.0f / smoothTime;
    const float x = omega * deltaTime;
    const float exponential = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
    float change = current - target;
    const float originalTarget = target;
    const float maximumChange = maximumSpeed * smoothTime;
    change = clamp(change, -maximumChange, maximumChange);
    target = current - change;
    const float temporary = (velocity + omega * change) * deltaTime;
    velocity = (velocity - omega * temporary) * exponential;
    float output = target + (change + temporary) * exponential;
    if ((originalTarget - current > 0.0f) == (output > originalTarget)) {
        output = originalTarget;
        velocity = deltaTime > 0.0f ? (output - originalTarget) / deltaTime : 0.0f;
    }
    return output;
}

} // namespace peco::math
