#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include <peco\matrix3.hpp>
#include <peco\rect.hpp>

namespace peco {

struct PathContour {
    std::vector<Vec2> points;
    bool closed = false;

    bool empty() const { return points.empty(); }
    std::size_t size() const { return points.size(); }
    const Vec2& front() const { return points.front(); }
    const Vec2& back() const { return points.back(); }
    float length() const
    {
        float result = 0.0f;
        for (std::size_t index = 1; index < points.size(); ++index) {
            result += Vec2::distance(points[index - 1], points[index]);
        }
        if (closed && points.size() > 2) {
            result += Vec2::distance(points.back(), points.front());
        }
        return result;
    }
    Rect bounds() const
    {
        if (points.empty()) {
            return {};
        }
        Vec2 minimum = points.front();
        Vec2 maximum = points.front();
        for (const Vec2& point : points) {
            minimum = Vec2::min(minimum, point);
            maximum = Vec2::max(maximum, point);
        }
        return Rect::fromEdges(minimum.x, minimum.y, maximum.x, maximum.y);
    }
    Vec2 sample(float normalizedDistance) const
    {
        if (points.empty()) {
            return {};
        }
        if (points.size() == 1) {
            return points.front();
        }
        const float total = length();
        if (total <= 0.0f) {
            return points.front();
        }
        float remaining = std::clamp(normalizedDistance, 0.0f, 1.0f) * total;
        for (std::size_t index = 1; index < points.size(); ++index) {
            const Vec2 delta = points[index] - points[index - 1];
            const float segmentLength = delta.length();
            if (remaining <= segmentLength) {
                return points[index - 1]
                    + delta * (segmentLength > 0.0f ? remaining / segmentLength : 0.0f);
            }
            remaining -= segmentLength;
        }
        if (closed) {
            const Vec2 delta = points.front() - points.back();
            const float segmentLength = delta.length();
            return points.back()
                + delta * (segmentLength > 0.0f ? remaining / segmentLength : 0.0f);
        }
        return points.back();
    }
};

class Path2D {
public:
    enum class Verb {
        Move,
        Line,
        Quadratic,
        Cubic,
        Close
    };
    struct Command {
        Verb verb = Verb::Move;
        Vec2 first{};
        Vec2 second{};
        Vec2 third{};
    };

    Path2D& moveTo(const Vec2& point)
    {
        commands_.push_back({Verb::Move, point});
        return *this;
    }
    Path2D& lineTo(const Vec2& point)
    {
        commands_.push_back({Verb::Line, point});
        return *this;
    }
    Path2D& quadraticTo(const Vec2& control, const Vec2& end)
    {
        commands_.push_back({Verb::Quadratic, control, end});
        return *this;
    }
    Path2D& cubicTo(const Vec2& firstControl, const Vec2& secondControl,
                    const Vec2& end)
    {
        commands_.push_back({Verb::Cubic, firstControl, secondControl, end});
        return *this;
    }
    Path2D& close()
    {
        commands_.push_back({Verb::Close});
        return *this;
    }
    Path2D& addRectangle(const Rect& rectangle)
    {
        moveTo({rectangle.left(), rectangle.top()});
        lineTo({rectangle.right(), rectangle.top()});
        lineTo({rectangle.right(), rectangle.bottom()});
        lineTo({rectangle.left(), rectangle.bottom()});
        return close();
    }
    Path2D& addPolygon(const std::vector<Vec2>& points, bool closePath = true)
    {
        if (points.empty()) {
            return *this;
        }
        moveTo(points.front());
        for (std::size_t index = 1; index < points.size(); ++index) {
            lineTo(points[index]);
        }
        if (closePath) {
            close();
        }
        return *this;
    }
    Path2D& addEllipse(const Vec2& center, const Vec2& radii,
                       int segments = 48)
    {
        if (segments < 3 || radii.x < 0.0f || radii.y < 0.0f) {
            return *this;
        }
        constexpr float fullTurn = 6.28318530717958647692f;
        for (int index = 0; index < segments; ++index) {
            const float angle = fullTurn * static_cast<float>(index)
                / static_cast<float>(segments);
            const Vec2 point{center.x + std::cos(angle) * radii.x,
                             center.y + std::sin(angle) * radii.y};
            if (index == 0) {
                moveTo(point);
            } else {
                lineTo(point);
            }
        }
        return close();
    }
    Path2D& addArc(const Vec2& center, float radius,
                   float startRadians, float endRadians, int segments = 32)
    {
        if (radius < 0.0f || segments < 1) {
            return *this;
        }
        const float sweep = endRadians - startRadians;
        const int count = std::max(1, static_cast<int>(
            std::ceil(std::fabs(sweep) / 6.28318530717958647692f * segments)));
        for (int index = 0; index <= count; ++index) {
            const float amount = static_cast<float>(index) / static_cast<float>(count);
            const float angle = startRadians + sweep * amount;
            const Vec2 point{center.x + std::cos(angle) * radius,
                             center.y + std::sin(angle) * radius};
            if (index == 0) {
                moveTo(point);
            } else {
                lineTo(point);
            }
        }
        return *this;
    }
    Path2D& append(const Path2D& other)
    {
        commands_.insert(commands_.end(), other.commands_.begin(), other.commands_.end());
        return *this;
    }
    Path2D& clear()
    {
        commands_.clear();
        return *this;
    }
    bool empty() const { return commands_.empty(); }
    std::size_t commandCount() const { return commands_.size(); }
    const std::vector<Command>& commands() const { return commands_; }

    std::vector<PathContour> flatten(float tolerance = 0.5f,
                                     int maximumSegments = 128) const
    {
        std::vector<PathContour> contours;
        PathContour* current = nullptr;
        Vec2 cursor{};
        Vec2 start{};
        const float safeTolerance = std::max(tolerance, 0.01f);
        const int safeMaximum = std::max(maximumSegments, 2);

        for (const Command& command : commands_) {
            switch (command.verb) {
            case Verb::Move:
                contours.push_back({});
                current = &contours.back();
                current->points.push_back(command.first);
                cursor = start = command.first;
                break;
            case Verb::Line:
                if (current == nullptr) {
                    contours.push_back({});
                    current = &contours.back();
                    current->points.push_back(cursor);
                    start = cursor;
                }
                current->points.push_back(command.first);
                cursor = command.first;
                break;
            case Verb::Quadratic:
                if (current == nullptr) {
                    contours.push_back({});
                    current = &contours.back();
                    current->points.push_back(cursor);
                    start = cursor;
                }
                flattenQuadratic(*current, cursor, command.first, command.second,
                                 safeTolerance, safeMaximum);
                cursor = command.second;
                break;
            case Verb::Cubic:
                if (current == nullptr) {
                    contours.push_back({});
                    current = &contours.back();
                    current->points.push_back(cursor);
                    start = cursor;
                }
                flattenCubic(*current, cursor, command.first, command.second,
                             command.third, safeTolerance, safeMaximum);
                cursor = command.third;
                break;
            case Verb::Close:
                if (current != nullptr && !current->points.empty()) {
                    current->closed = true;
                    cursor = start;
                    current = nullptr;
                }
                break;
            }
        }
        return contours;
    }

    Path2D transformed(const Mat3& matrix) const
    {
        Path2D result;
        result.commands_.reserve(commands_.size());
        for (const Command& command : commands_) {
            result.commands_.push_back({
                command.verb,
                matrix.transformPoint(command.first),
                matrix.transformPoint(command.second),
                matrix.transformPoint(command.third)
            });
        }
        return result;
    }

    Rect bounds(float tolerance = 0.5f) const
    {
        Vec2 minimum{};
        Vec2 maximum{};
        bool hasPoint = false;
        for (const PathContour& contour : flatten(tolerance)) {
            for (const Vec2& point : contour.points) {
                if (!hasPoint) {
                    minimum = maximum = point;
                    hasPoint = true;
                } else {
                    minimum = Vec2::min(minimum, point);
                    maximum = Vec2::max(maximum, point);
                }
            }
        }
        return hasPoint ? Rect::fromEdges(minimum.x, minimum.y, maximum.x, maximum.y)
                        : Rect{};
    }

private:
    static int segmentCount(float controlLength, float tolerance, int maximumSegments)
    {
        const float estimate = std::ceil(
            std::sqrt(std::max(controlLength, 1.0f) / tolerance));
        return static_cast<int>(std::clamp(estimate, 2.0f,
                                           static_cast<float>(maximumSegments)));
    }
    static void flattenQuadratic(PathContour& contour, const Vec2& start,
                                const Vec2& control, const Vec2& end,
                                float tolerance, int maximumSegments)
    {
        const float length = Vec2::distance(start, control) + Vec2::distance(control, end);
        const int steps = segmentCount(length, tolerance, maximumSegments);
        for (int index = 1; index <= steps; ++index) {
            const float t = static_cast<float>(index) / static_cast<float>(steps);
            const float inverse = 1.0f - t;
            contour.points.push_back(start * (inverse * inverse)
                + control * (2.0f * inverse * t) + end * (t * t));
        }
    }
    static void flattenCubic(PathContour& contour, const Vec2& start,
                             const Vec2& firstControl, const Vec2& secondControl,
                             const Vec2& end, float tolerance, int maximumSegments)
    {
        const float length = Vec2::distance(start, firstControl)
            + Vec2::distance(firstControl, secondControl)
            + Vec2::distance(secondControl, end);
        const int steps = segmentCount(length, tolerance, maximumSegments);
        for (int index = 1; index <= steps; ++index) {
            const float t = static_cast<float>(index) / static_cast<float>(steps);
            const float inverse = 1.0f - t;
            contour.points.push_back(start * (inverse * inverse * inverse)
                + firstControl * (3.0f * inverse * inverse * t)
                + secondControl * (3.0f * inverse * t * t)
                + end * (t * t * t));
        }
    }

    std::vector<Command> commands_;
};

} // namespace peco
