#pragma once

#include <algorithm>
#include <cmath>

#include <peco\rect.hpp>
#include <peco\ray.hpp>

namespace peco {

struct Segment2 {
    Vec2 start{};
    Vec2 end{};

    constexpr Vec2 direction() const { return end - start; }
    float length() const { return direction().length(); }
    constexpr Vec2 pointAt(float amount) const
    {
        return start + (end - start) * amount;
    }
    float closestParameter(const Vec2& point) const
    {
        const Vec2 delta = direction();
        const float denominator = delta.lengthSquared();
        return denominator <= 0.0f ? 0.0f
            : std::clamp((point - start).dot(delta) / denominator, 0.0f, 1.0f);
    }
    Vec2 closestPoint(const Vec2& point) const
    {
        return pointAt(closestParameter(point));
    }
    float distanceTo(const Vec2& point) const
    {
        return Vec2::distance(point, closestPoint(point));
    }
    Rect bounds() const
    {
        const Vec2 low = Vec2::min(start, end);
        const Vec2 high = Vec2::max(start, end);
        return Rect::fromEdges(low.x, low.y, high.x, high.y);
    }
};

struct Circle2 {
    Vec2 center{};
    float radius = 1.0f;

    constexpr float diameter() const { return radius * 2.0f; }
    constexpr float area() const { return 3.14159265358979323846f * radius * radius; }
    constexpr bool contains(const Vec2& point) const
    {
        return Vec2::distanceSquared(center, point) <= radius * radius;
    }
    bool intersects(const Circle2& other) const
    {
        const float combinedRadius = radius + other.radius;
        return Vec2::distanceSquared(center, other.center)
            <= combinedRadius * combinedRadius;
    }
    bool intersects(const Rect& rectangle) const
    {
        return Vec2::distanceSquared(center, rectangle.clampPoint(center))
            <= radius * radius;
    }
    Rect bounds() const
    {
        return {center.x - radius, center.y - radius, diameter(), diameter()};
    }
};

struct Triangle2 {
    Vec2 a{};
    Vec2 b{};
    Vec2 c{};

    float signedArea() const { return (b - a).cross(c - a) * 0.5f; }
    float area() const { return std::fabs(signedArea()); }
    Vec2 centroid() const { return (a + b + c) / 3.0f; }
    Vec2 normal() const { return (b - a).perpendicular().normalized(); }
    bool contains(const Vec2& point, float epsilon = 0.00001f) const
    {
        const float denominator = signedArea() * 2.0f;
        if (std::fabs(denominator) <= epsilon) {
            return false;
        }
        const float first = (b - a).cross(point - a) / denominator;
        const float second = (c - b).cross(point - b) / denominator;
        const float third = (a - c).cross(point - c) / denominator;
        return first >= -epsilon && second >= -epsilon && third >= -epsilon;
    }
    Rect bounds() const
    {
        const Vec2 low = Vec2::min(a, Vec2::min(b, c));
        const Vec2 high = Vec2::max(a, Vec2::max(b, c));
        return Rect::fromEdges(low.x, low.y, high.x, high.y);
    }
};

struct Capsule2 {
    Segment2 segment{};
    float radius = 0.5f;

    bool contains(const Vec2& point) const
    {
        return segment.distanceTo(point) <= radius;
    }
    bool intersects(const Circle2& circle) const
    {
        const float combined = radius + circle.radius;
        return segment.distanceTo(circle.center) <= combined;
    }
    Rect bounds() const
    {
        const Vec2 low = Vec2::min(segment.start, segment.end);
        const Vec2 high = Vec2::max(segment.start, segment.end);
        return Rect::fromEdges(low.x - radius, low.y - radius,
                               high.x + radius, high.y + radius);
    }
};

inline bool intersect(const Segment2& lhs, const Segment2& rhs,
                      Vec2& point, float epsilon = 0.00001f)
{
    const Vec2 first = lhs.end - lhs.start;
    const Vec2 second = rhs.end - rhs.start;
    const float denominator = first.cross(second);
    if (std::fabs(denominator) <= epsilon) {
        return false;
    }
    const Vec2 offset = rhs.start - lhs.start;
    const float t = offset.cross(second) / denominator;
    const float u = offset.cross(first) / denominator;
    if (t < 0.0f || t > 1.0f || u < 0.0f || u > 1.0f) {
        return false;
    }
    point = lhs.pointAt(t);
    return true;
}

inline bool overlaps(const Circle2& circle, const Triangle2& triangle)
{
    return triangle.contains(circle.center)
        || Vec2::distanceSquared(circle.center, Segment2{triangle.a, triangle.b}.closestPoint(circle.center))
            <= circle.radius * circle.radius
        || Vec2::distanceSquared(circle.center, Segment2{triangle.b, triangle.c}.closestPoint(circle.center))
            <= circle.radius * circle.radius
        || Vec2::distanceSquared(circle.center, Segment2{triangle.c, triangle.a}.closestPoint(circle.center))
            <= circle.radius * circle.radius;
}

inline bool pointInPolygon(const Vec2& point, const Vec2* vertices, std::size_t count)
{
    if (vertices == nullptr || count < 3) {
        return false;
    }
    bool inside = false;
    for (std::size_t current = 0, previous = count - 1;
         current < count; previous = current++) {
        const Vec2& a = vertices[current];
        const Vec2& b = vertices[previous];
        const bool crosses = ((a.y > point.y) != (b.y > point.y))
            && (point.x < (b.x - a.x) * (point.y - a.y)
                / (b.y - a.y) + a.x);
        if (crosses) {
            inside = !inside;
        }
    }
    return inside;
}

inline float signedPolygonArea(const Vec2* vertices, std::size_t count)
{
    if (vertices == nullptr || count < 3) {
        return 0.0f;
    }
    float doubleArea = 0.0f;
    for (std::size_t index = 0; index < count; ++index) {
        const Vec2& first = vertices[index];
        const Vec2& second = vertices[(index + 1) % count];
        doubleArea += first.cross(second);
    }
    return doubleArea * 0.5f;
}

} // namespace peco
