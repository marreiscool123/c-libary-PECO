#pragma once

#include <array>
#include <limits>

#include <peco\rect.hpp>

namespace peco {

class Bounds2 {
public:
    constexpr Bounds2() = default;
    constexpr Bounds2(const Vec2& minimum, const Vec2& maximum)
        : minimum_(minimum), maximum_(maximum) {}

    static Bounds2 empty()
    {
        const float infinity = std::numeric_limits<float>::infinity();
        return {{infinity, infinity}, {-infinity, -infinity}};
    }
    static constexpr Bounds2 fromRect(const Rect& rectangle)
    {
        return {{rectangle.left(), rectangle.top()},
                {rectangle.right(), rectangle.bottom()}};
    }
    static constexpr Bounds2 fromCenterExtents(const Vec2& center, const Vec2& extents)
    {
        return {center - extents, center + extents};
    }
    constexpr const Vec2& minimum() const { return minimum_; }
    constexpr const Vec2& maximum() const { return maximum_; }
    constexpr Vec2 center() const { return (minimum_ + maximum_) * 0.5f; }
    constexpr Vec2 size() const { return maximum_ - minimum_; }
    constexpr Vec2 extents() const { return size() * 0.5f; }
    constexpr float area() const { return size().x * size().y; }
    constexpr bool isEmpty() const
    {
        return minimum_.x > maximum_.x || minimum_.y > maximum_.y;
    }
    constexpr bool contains(const Vec2& point) const
    {
        return !isEmpty() && point.x >= minimum_.x && point.x <= maximum_.x
            && point.y >= minimum_.y && point.y <= maximum_.y;
    }
    constexpr bool contains(const Bounds2& other) const
    {
        return !isEmpty() && !other.isEmpty()
            && contains(other.minimum_) && contains(other.maximum_);
    }
    constexpr bool intersects(const Bounds2& other) const
    {
        return !isEmpty() && !other.isEmpty()
            && minimum_.x <= other.maximum_.x && maximum_.x >= other.minimum_.x
            && minimum_.y <= other.maximum_.y && maximum_.y >= other.minimum_.y;
    }
    constexpr Vec2 closestPoint(const Vec2& point) const
    {
        return point.clamp(minimum_, maximum_);
    }
    constexpr Bounds2 expanded(const Vec2& point) const
    {
        return isEmpty() ? Bounds2{point, point}
                         : Bounds2{Vec2::min(minimum_, point), Vec2::max(maximum_, point)};
    }
    constexpr Bounds2 united(const Bounds2& other) const
    {
        return isEmpty() ? other
            : (other.isEmpty() ? *this
                : Bounds2{Vec2::min(minimum_, other.minimum_),
                          Vec2::max(maximum_, other.maximum_)});
    }
    constexpr Bounds2 intersection(const Bounds2& other) const
    {
        const Vec2 low = Vec2::max(minimum_, other.minimum_);
        const Vec2 high = Vec2::min(maximum_, other.maximum_);
        return high.x < low.x || high.y < low.y
            ? Bounds2::empty() : Bounds2{low, high};
    }
    constexpr Bounds2 translated(const Vec2& offset) const
    {
        return {minimum_ + offset, maximum_ + offset};
    }
    constexpr Bounds2 inflated(const Vec2& amount) const
    {
        return {minimum_ - amount, maximum_ + amount};
    }
    constexpr Rect rect() const
    {
        return Rect::fromEdges(minimum_.x, minimum_.y, maximum_.x, maximum_.y);
    }
    constexpr std::array<Vec2, 4> corners() const
    {
        return {{{minimum_.x, minimum_.y}, {maximum_.x, minimum_.y},
                 {maximum_.x, maximum_.y}, {minimum_.x, maximum_.y}}};
    }

private:
    Vec2 minimum_{};
    Vec2 maximum_{};
};

} // namespace peco
