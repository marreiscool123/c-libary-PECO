#pragma once

#include <algorithm>
#include <cmath>

#include <peco\vector2.hpp>

namespace peco {

struct Size2 {
    float width = 0.0f;
    float height = 0.0f;

    constexpr Size2() = default;
    constexpr Size2(float widthValue, float heightValue)
        : width(widthValue), height(heightValue) {}
    constexpr float area() const { return width * height; }
    constexpr Vec2 vector() const { return {width, height}; }
    constexpr bool isEmpty() const { return width <= 0.0f || height <= 0.0f; }
    constexpr Size2 operator+(const Size2& rhs) const
    {
        return {width + rhs.width, height + rhs.height};
    }
    constexpr Size2 operator-(const Size2& rhs) const
    {
        return {width - rhs.width, height - rhs.height};
    }
    constexpr Size2 operator*(float scale) const
    {
        return {width * scale, height * scale};
    }
};

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    constexpr Rect() = default;
    constexpr Rect(float xValue, float yValue, float widthValue, float heightValue)
        : x(xValue), y(yValue), width(widthValue), height(heightValue) {}

    static constexpr Rect fromEdges(float left, float top, float right, float bottom)
    {
        return {left, top, right - left, bottom - top};
    }
    constexpr float left() const { return x; }
    constexpr float top() const { return y; }
    constexpr float right() const { return x + width; }
    constexpr float bottom() const { return y + height; }
    constexpr Vec2 position() const { return {x, y}; }
    constexpr Size2 size() const { return {width, height}; }
    constexpr Vec2 center() const { return {x + width * 0.5f, y + height * 0.5f}; }
    constexpr bool isEmpty() const { return width <= 0.0f || height <= 0.0f; }
    constexpr bool contains(const Vec2& point) const
    {
        return point.x >= left() && point.x < right()
            && point.y >= top() && point.y < bottom();
    }
    constexpr bool containsInclusive(const Vec2& point) const
    {
        return point.x >= left() && point.x <= right()
            && point.y >= top() && point.y <= bottom();
    }
    constexpr bool contains(const Rect& other) const
    {
        return other.left() >= left() && other.right() <= right()
            && other.top() >= top() && other.bottom() <= bottom();
    }
    constexpr bool intersects(const Rect& other) const
    {
        return left() < other.right() && right() > other.left()
            && top() < other.bottom() && bottom() > other.top();
    }
    constexpr Rect translated(const Vec2& offset) const
    {
        return {x + offset.x, y + offset.y, width, height};
    }
    constexpr Rect inflated(float horizontal, float vertical) const
    {
        return {x - horizontal, y - vertical,
                width + 2.0f * horizontal, height + 2.0f * vertical};
    }
    constexpr Rect deflated(float horizontal, float vertical) const
    {
        return inflated(-horizontal, -vertical);
    }
    constexpr Rect withPosition(const Vec2& positionValue) const
    {
        return {positionValue.x, positionValue.y, width, height};
    }
    constexpr Rect withSize(const Size2& sizeValue) const
    {
        return {x, y, sizeValue.width, sizeValue.height};
    }
    constexpr Vec2 clampPoint(const Vec2& point) const
    {
        return point.clamp({left(), top()}, {right(), bottom()});
    }
    constexpr Rect intersection(const Rect& other) const
    {
        const float l = left() > other.left() ? left() : other.left();
        const float t = top() > other.top() ? top() : other.top();
        const float r = right() < other.right() ? right() : other.right();
        const float b = bottom() < other.bottom() ? bottom() : other.bottom();
        return {l, t, r > l ? r - l : 0.0f, b > t ? b - t : 0.0f};
    }
    constexpr Rect united(const Rect& other) const
    {
        if (isEmpty()) {
            return other;
        }
        if (other.isEmpty()) {
            return *this;
        }
        const float l = left() < other.left() ? left() : other.left();
        const float t = top() < other.top() ? top() : other.top();
        const float r = right() > other.right() ? right() : other.right();
        const float b = bottom() > other.bottom() ? bottom() : other.bottom();
        return fromEdges(l, t, r, b);
    }
    Rect normalized() const
    {
        const float l = std::min(left(), right());
        const float r = std::max(left(), right());
        const float t = std::min(top(), bottom());
        const float b = std::max(top(), bottom());
        return fromEdges(l, t, r, b);
    }
    float distanceTo(const Vec2& point) const
    {
        return Vec2::distance(point, clampPoint(point));
    }
};

} // namespace peco
