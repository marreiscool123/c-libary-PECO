#pragma once

#include <cmath>
#include <limits>

#include <peco\bounds3.hpp>
#include <peco\rect.hpp>

namespace peco {

struct Ray2 {
    Vec2 origin{};
    Vec2 direction{1.0f, 0.0f};

    constexpr Vec2 at(float distance) const { return origin + direction * distance; }
    float closestParameter(const Vec2& point) const
    {
        const float denominator = direction.lengthSquared();
        return denominator == 0.0f ? 0.0f
            : (point - origin).dot(direction) / denominator;
    }
    Vec2 closestPoint(const Vec2& point) const
    {
        return at(std::fmax(0.0f, closestParameter(point)));
    }
    float distanceTo(const Vec2& point) const
    {
        return Vec2::distance(point, closestPoint(point));
    }
};

struct Ray3 {
    Vec3 origin{};
    Vec3 direction{0.0f, 0.0f, -1.0f};

    constexpr Vec3 at(float distance) const { return origin + direction * distance; }
    float closestParameter(const Vec3& point) const
    {
        const float denominator = direction.lengthSquared();
        return denominator == 0.0f ? 0.0f
            : (point - origin).dot(direction) / denominator;
    }
    Vec3 closestPoint(const Vec3& point) const
    {
        return at(std::fmax(0.0f, closestParameter(point)));
    }
    float distanceTo(const Vec3& point) const
    {
        return Vec3::distance(point, closestPoint(point));
    }
    Ray3 normalized() const { return {origin, direction.normalized()}; }
};

struct Plane {
    Vec3 normal{0.0f, 1.0f, 0.0f};
    float distance = 0.0f;

    constexpr Plane() = default;
    constexpr Plane(const Vec3& normalValue, float distanceValue)
        : normal(normalValue), distance(distanceValue) {}
    static Plane fromPointNormal(const Vec3& point, const Vec3& normalValue)
    {
        const Vec3 unit = normalValue.normalized();
        return {unit, -unit.dot(point)};
    }
    static Plane fromPoints(const Vec3& a, const Vec3& b, const Vec3& c)
    {
        const Vec3 normalValue = (b - a).cross(c - a).normalized();
        return {normalValue, -normalValue.dot(a)};
    }
    constexpr float signedDistance(const Vec3& point) const
    {
        return normal.dot(point) + distance;
    }
    Vec3 project(const Vec3& point) const
    {
        return point - normal * signedDistance(point);
    }
    Vec3 reflect(const Vec3& point) const
    {
        return point - normal * (2.0f * signedDistance(point));
    }
    Plane normalized() const
    {
        const float magnitude = normal.length();
        return magnitude > 0.0f
            ? Plane{normal / magnitude, distance / magnitude}
            : Plane{};
    }
    bool raycast(const Ray3& ray, float& distanceAlongRay,
                 float epsilon = 0.00001f) const
    {
        const float denominator = normal.dot(ray.direction);
        if (std::fabs(denominator) <= epsilon) {
            return false;
        }
        distanceAlongRay = -(normal.dot(ray.origin) + distance) / denominator;
        return distanceAlongRay >= 0.0f;
    }
};

struct RayHit {
    float distance = std::numeric_limits<float>::infinity();
    Vec3 point{};
    Vec3 normal{};
    bool hit = false;
};

inline bool intersect(const Ray3& ray, const Bounds3& bounds,
                      float& entryDistance, float& exitDistance)
{
    if (bounds.isEmpty()) {
        return false;
    }
    float nearValue = 0.0f;
    float farValue = std::numeric_limits<float>::infinity();
    for (int axis = 0; axis < 3; ++axis) {
        const float origin = axis == 0 ? ray.origin.x : (axis == 1 ? ray.origin.y : ray.origin.z);
        const float direction = axis == 0 ? ray.direction.x : (axis == 1 ? ray.direction.y : ray.direction.z);
        const float minimum = axis == 0 ? bounds.minimum.x : (axis == 1 ? bounds.minimum.y : bounds.minimum.z);
        const float maximum = axis == 0 ? bounds.maximum.x : (axis == 1 ? bounds.maximum.y : bounds.maximum.z);
        if (std::fabs(direction) < 0.0000001f) {
            if (origin < minimum || origin > maximum) {
                return false;
            }
            continue;
        }
        float first = (minimum - origin) / direction;
        float second = (maximum - origin) / direction;
        if (first > second) {
            const float temporary = first;
            first = second;
            second = temporary;
        }
        nearValue = std::fmax(nearValue, first);
        farValue = std::fmin(farValue, second);
        if (nearValue > farValue) {
            return false;
        }
    }
    entryDistance = nearValue;
    exitDistance = farValue;
    return farValue >= 0.0f;
}

inline bool intersect(const Ray3& ray, const Plane& plane, float& distanceAlongRay)
{
    return plane.raycast(ray, distanceAlongRay);
}

inline bool intersect(const Ray3& ray, const Vec3& center, float radius,
                      float& distanceAlongRay)
{
    const Vec3 offset = ray.origin - center;
    const float a = ray.direction.dot(ray.direction);
    const float b = 2.0f * offset.dot(ray.direction);
    const float c = offset.dot(offset) - radius * radius;
    const float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f || a == 0.0f) {
        return false;
    }
    const float root = std::sqrt(discriminant);
    const float first = (-b - root) / (2.0f * a);
    const float second = (-b + root) / (2.0f * a);
    distanceAlongRay = first >= 0.0f ? first : second;
    return distanceAlongRay >= 0.0f;
}

inline bool intersect(const Ray2& ray, const Rect& rectangle, float& distanceAlongRay)
{
    const float origins[2]{ray.origin.x, ray.origin.y};
    const float directions[2]{ray.direction.x, ray.direction.y};
    const float minimums[2]{rectangle.left(), rectangle.top()};
    const float maximums[2]{rectangle.right(), rectangle.bottom()};
    float entry = 0.0f;
    float exit = std::numeric_limits<float>::infinity();
    for (int axis = 0; axis < 2; ++axis) {
        if (std::fabs(directions[axis]) < 0.0000001f) {
            if (origins[axis] < minimums[axis] || origins[axis] > maximums[axis]) {
                return false;
            }
            continue;
        }
        float first = (minimums[axis] - origins[axis]) / directions[axis];
        float second = (maximums[axis] - origins[axis]) / directions[axis];
        if (first > second) {
            const float swap = first;
            first = second;
            second = swap;
        }
        entry = std::fmax(entry, first);
        exit = std::fmin(exit, second);
        if (entry > exit) {
            return false;
        }
    }
    if (exit < 0.0f) {
        return false;
    }
    distanceAlongRay = entry;
    return true;
}

inline bool intersectSegment(const Vec2& a, const Vec2& b,
                             const Vec2& c, const Vec2& d,
                             Vec2& intersectionPoint,
                             float epsilon = 0.00001f)
{
    const Vec2 r = b - a;
    const Vec2 s = d - c;
    const float denominator = r.cross(s);
    if (std::fabs(denominator) <= epsilon) {
        return false;
    }
    const Vec2 offset = c - a;
    const float t = offset.cross(s) / denominator;
    const float u = offset.cross(r) / denominator;
    if (t < 0.0f || t > 1.0f || u < 0.0f || u > 1.0f) {
        return false;
    }
    intersectionPoint = a + r * t;
    return true;
}

} // namespace peco
