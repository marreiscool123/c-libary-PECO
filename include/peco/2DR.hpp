#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include <peco\color.hpp>
#include <peco\image.hpp>
#include <peco\imageL2D.hpp>
#include <peco\rect.hpp>
#include <peco\transform2d.hpp>
#include <peco\vector2.hpp>

namespace peco {

enum class BlendMode2D {
    Alpha,
    Replace,
    Add,
    Multiply,
    Screen
};

enum class Sampling2D {
    Nearest,
    Bilinear
};

enum class Morphology2D {
    Dilate,
    Erode
};

struct GradientStop2D {
    float position = 0.0f;
    Color color = Color::white();
};

struct StrokeStyle2D {
    Color color = Color::white();
    float width = 1.0f;
    bool roundCaps = true;
    bool roundJoins = true;
};

class Renderer2D {
public:
    Renderer2D() = default;
    explicit Renderer2D(Image& target) { setTarget(target); }

    void setTarget(Image& target)
    {
        target_ = &target;
        clips_.clear();
    }
    Image* target() { return target_; }
    const Image* target() const { return target_; }
    int width() const { return target_ == nullptr ? 0 : target_->width(); }
    int height() const { return target_ == nullptr ? 0 : target_->height(); }
    bool ready() const { return target_ != nullptr && !target_->empty(); }
    BlendMode2D blendMode() const { return blendMode_; }
    void setBlendMode(BlendMode2D value) { blendMode_ = value; }
    Sampling2D sampling() const { return sampling_; }
    void setSampling(Sampling2D value) { sampling_ = value; }
    std::size_t clipDepth() const { return clips_.size(); }

    bool pushClip(const Rect& rectangle)
    {
        if (!finiteRect(rectangle)) {
            return false;
        }
        Rect bounds{0.0f, 0.0f, static_cast<float>(width()),
                    static_cast<float>(height())};
        Rect clipped = rectangle.normalized().intersection(bounds);
        if (!clips_.empty()) {
            clipped = clipped.intersection(clips_.back());
        }
        clips_.push_back(clipped);
        return !clipped.isEmpty();
    }
    bool popClip()
    {
        if (clips_.empty()) {
            return false;
        }
        clips_.pop_back();
        return true;
    }
    void resetClip() { clips_.clear(); }
    Rect clipBounds() const
    {
        if (!clips_.empty()) {
            return clips_.back();
        }
        return {0.0f, 0.0f, static_cast<float>(width()),
                static_cast<float>(height())};
    }

    void clear(Color color)
    {
        if (ready()) {
            target_->clear(color);
        }
    }
    bool putPixel(int x, int y, Color color)
    {
        if (!visible(x, y)) {
            return false;
        }
        blend(target_->at(x, y), color);
        return true;
    }
    bool getPixel(int x, int y, Color& color) const
    {
        return ready() && target_->getPixel(x, y, color);
    }

    void line(const Vec2& from, const Vec2& to, Color color, float thickness = 1.0f)
    {
        strokeLine(from, to, {color, thickness, true, true});
    }
    void strokeLine(const Vec2& from, const Vec2& to, const StrokeStyle2D& style)
    {
        if (!ready() || !from.isFinite() || !to.isFinite()
            || !std::isfinite(style.width) || style.width <= 0.0f) {
            return;
        }
        const Vec2 delta = to - from;
        const float lengthSquared = delta.lengthSquared();
        const float radius = style.width * 0.5f;
        const Rect area = segmentBounds(from, to, radius).intersection(clipBounds());
        const int left = pixelStart(area.left());
        const int top = pixelStart(area.top());
        const int right = pixelEnd(area.right(), width());
        const int bottom = pixelEnd(area.bottom(), height());
        for (int y = top; y < bottom; ++y) {
            for (int x = left; x < right; ++x) {
                const Vec2 point{static_cast<float>(x) + 0.5f,
                                 static_cast<float>(y) + 0.5f};
                const float along = lengthSquared > 0.0f
                    ? (point - from).dot(delta) / lengthSquared : 0.0f;
                const float segmentLength = std::sqrt(lengthSquared);
                if (!style.roundCaps && segmentLength > 0.0f
                    && (along < -radius / segmentLength
                        || along > 1.0f + radius / segmentLength)) {
                    continue;
                }
                const float clamped = std::clamp(along, 0.0f, 1.0f);
                const Vec2 closest = from + delta * clamped;
                if (Vec2::distanceSquared(point, closest) <= radius * radius) {
                    putPixel(x, y, style.color);
                }
            }
        }
    }
    void polyline(const std::vector<Vec2>& points, Color color,
                  float thickness = 1.0f, bool closed = false)
    {
        if (points.size() < 2) {
            return;
        }
        for (std::size_t i = 1; i < points.size(); ++i) {
            line(points[i - 1], points[i], color, thickness);
        }
        if (closed) {
            line(points.back(), points.front(), color, thickness);
        }
    }
    void polyline(const Vec2* points, std::size_t count, Color color,
                  float thickness = 1.0f, bool closed = false)
    {
        if (points == nullptr || count < 2) {
            return;
        }
        for (std::size_t i = 1; i < count; ++i) {
            line(points[i - 1], points[i], color, thickness);
        }
        if (closed) {
            line(points[count - 1], points[0], color, thickness);
        }
    }
    void dashedLine(const Vec2& from, const Vec2& to, Color color,
                    float thickness, float dashLength, float gapLength)
    {
        const Vec2 delta = to - from;
        const float length = delta.length();
        const float period = dashLength + gapLength;
        if (!std::isfinite(length) || dashLength <= 0.0f || gapLength < 0.0f
            || period <= 0.0f) {
            return;
        }
        for (float offset = 0.0f; offset < length; offset += period) {
            const float end = std::min(length, offset + dashLength);
            line(from + delta * (offset / std::max(length, 1.0e-20f)),
                 from + delta * (end / std::max(length, 1.0e-20f)),
                 color, thickness);
        }
    }

    void fillRect(const Rect& rectangle, Color color)
    {
        if (!ready() || !finiteRect(rectangle)) {
            return;
        }
        const Rect area = rectangle.normalized().intersection(clipBounds());
        const int left = pixelStart(area.left());
        const int top = pixelStart(area.top());
        const int right = pixelEnd(area.right(), width());
        const int bottom = pixelEnd(area.bottom(), height());
        for (int y = top; y < bottom; ++y) {
            for (int x = left; x < right; ++x) {
                putPixel(x, y, color);
            }
        }
    }
    void rect(const Rect& rectangle, Color color, float thickness = 1.0f)
    {
        const Rect normalized = rectangle.normalized();
        line({normalized.left(), normalized.top()},
             {normalized.right(), normalized.top()}, color, thickness);
        line({normalized.right(), normalized.top()},
             {normalized.right(), normalized.bottom()}, color, thickness);
        line({normalized.right(), normalized.bottom()},
             {normalized.left(), normalized.bottom()}, color, thickness);
        line({normalized.left(), normalized.bottom()},
             {normalized.left(), normalized.top()}, color, thickness);
    }
    void roundedRect(const Rect& rectangle, float radius, Color color,
                     float thickness = 1.0f)
    {
        if (!finiteRect(rectangle) || !std::isfinite(radius)) {
            return;
        }
        const Rect r = rectangle.normalized();
        const float limit = std::min(r.width, r.height) * 0.5f;
        radius = std::clamp(radius, 0.0f, limit);
        if (radius <= 0.0f) {
            rect(r, color, thickness);
            return;
        }
        const float quarter = 1.57079632679489661923f;
        std::vector<Vec2> points;
        points.reserve(36);
        appendArc(points, {r.right() - radius, r.top() + radius},
                  radius, -quarter, 0.0f, 8);
        appendArc(points, {r.right() - radius, r.bottom() - radius},
                  radius, 0.0f, quarter, 8);
        appendArc(points, {r.left() + radius, r.bottom() - radius},
                  radius, quarter, quarter * 2.0f, 8);
        appendArc(points, {r.left() + radius, r.top() + radius},
                  radius, quarter * 2.0f, quarter * 3.0f, 8);
        polyline(points, color, thickness, true);
    }
    void fillRoundedRect(const Rect& rectangle, float radius, Color color)
    {
        if (!ready() || !finiteRect(rectangle) || !std::isfinite(radius)) {
            return;
        }
        const Rect r = rectangle.normalized().intersection(clipBounds());
        radius = std::clamp(radius, 0.0f, std::min(r.width, r.height) * 0.5f);
        const float radiusSquared = radius * radius;
        for (int y = pixelStart(r.top()); y < pixelEnd(r.bottom(), height()); ++y) {
            for (int x = pixelStart(r.left()); x < pixelEnd(r.right(), width()); ++x) {
                const float px = static_cast<float>(x) + 0.5f;
                const float py = static_cast<float>(y) + 0.5f;
                const float nearestX = std::clamp(px, r.left() + radius,
                                                   r.right() - radius);
                const float nearestY = std::clamp(py, r.top() + radius,
                                                   r.bottom() - radius);
                if (radius == 0.0f
                    || Vec2::distanceSquared({px, py}, {nearestX, nearestY})
                        <= radiusSquared) {
                    putPixel(x, y, color);
                }
            }
        }
    }
    void circle(const Vec2& center, float radius, Color color,
                float thickness = 1.0f)
    {
        ellipse(center, {radius, radius}, color, thickness);
    }
    void fillCircle(const Vec2& center, float radius, Color color)
    {
        fillEllipse(center, {radius, radius}, color);
    }
    void ellipse(const Vec2& center, const Vec2& radii,
                 Color color, float thickness = 1.0f)
    {
        if (!center.isFinite() || !radii.isFinite() || radii.x <= 0.0f
            || radii.y <= 0.0f || thickness <= 0.0f) {
            return;
        }
        const float outerX = radii.x + thickness * 0.5f;
        const float outerY = radii.y + thickness * 0.5f;
        const Rect bounds{center.x - outerX, center.y - outerY,
                          outerX * 2.0f, outerY * 2.0f};
        const Rect area = bounds.intersection(clipBounds());
        const float innerX = std::max(0.0f, radii.x - thickness * 0.5f);
        const float innerY = std::max(0.0f, radii.y - thickness * 0.5f);
        for (int y = pixelStart(area.top()); y < pixelEnd(area.bottom(), height()); ++y) {
            for (int x = pixelStart(area.left()); x < pixelEnd(area.right(), width()); ++x) {
                const float dx = (static_cast<float>(x) + 0.5f - center.x);
                const float dy = (static_cast<float>(y) + 0.5f - center.y);
                const float outer = dx * dx / (outerX * outerX)
                                  + dy * dy / (outerY * outerY);
                const float inner = innerX > 0.0f && innerY > 0.0f
                    ? dx * dx / (innerX * innerX) + dy * dy / (innerY * innerY)
                    : 2.0f;
                if (outer <= 1.0f && inner >= 1.0f) {
                    putPixel(x, y, color);
                }
            }
        }
    }
    void fillEllipse(const Vec2& center, const Vec2& radii, Color color)
    {
        if (!center.isFinite() || !radii.isFinite()
            || radii.x <= 0.0f || radii.y <= 0.0f) {
            return;
        }
        const Rect area = Rect{center.x - radii.x, center.y - radii.y,
                               radii.x * 2.0f, radii.y * 2.0f}
                              .intersection(clipBounds());
        for (int y = pixelStart(area.top()); y < pixelEnd(area.bottom(), height()); ++y) {
            for (int x = pixelStart(area.left()); x < pixelEnd(area.right(), width()); ++x) {
                const float dx = (static_cast<float>(x) + 0.5f - center.x) / radii.x;
                const float dy = (static_cast<float>(y) + 0.5f - center.y) / radii.y;
                if (dx * dx + dy * dy <= 1.0f) {
                    putPixel(x, y, color);
                }
            }
        }
    }

    void fillPolygon(const std::vector<Vec2>& points, Color color)
    {
        if (!ready() || points.size() < 3) {
            return;
        }
        float minY = std::numeric_limits<float>::infinity();
        float maxY = -std::numeric_limits<float>::infinity();
        for (const Vec2& point : points) {
            if (!point.isFinite()) {
                return;
            }
            minY = std::min(minY, point.y);
            maxY = std::max(maxY, point.y);
        }
        const Rect clip = clipBounds();
        const int firstY = pixelStart(std::max(minY, clip.top()));
        const int lastY = pixelEnd(std::min(maxY, clip.bottom()), height());
        std::vector<float> intersections;
        intersections.reserve(points.size());
        for (int y = firstY; y < lastY; ++y) {
            intersections.clear();
            const float scanY = static_cast<float>(y) + 0.5f;
            for (std::size_t i = 0; i < points.size(); ++i) {
                const Vec2& a = points[i];
                const Vec2& b = points[(i + 1) % points.size()];
                if ((a.y <= scanY && b.y > scanY)
                    || (b.y <= scanY && a.y > scanY)) {
                    const float amount = (scanY - a.y) / (b.y - a.y);
                    intersections.push_back(a.x + (b.x - a.x) * amount);
                }
            }
            std::sort(intersections.begin(), intersections.end());
            for (std::size_t i = 0; i + 1 < intersections.size(); i += 2) {
                const int firstX = pixelStart(std::max(intersections[i], clip.left()));
                const int lastX = pixelEnd(std::min(intersections[i + 1], clip.right()),
                                           width());
                for (int x = firstX; x < lastX; ++x) {
                    putPixel(x, y, color);
                }
            }
        }
    }
    void strokePolygon(const std::vector<Vec2>& points, Color color,
                       float thickness = 1.0f, bool close = true)
    {
        polyline(points, color, thickness, close);
    }
    void fillPolygonGradient(const std::vector<Vec2>& points,
                             const std::vector<Color>& vertexColors)
    {
        if (points.size() < 3 || points.size() != vertexColors.size()) {
            return;
        }
        for (std::size_t index = 1; index + 1 < points.size(); ++index) {
            gradientTriangle(points[0], vertexColors[0],
                             points[index], vertexColors[index],
                             points[index + 1], vertexColors[index + 1]);
        }
    }
    void triangle(const Vec2& a, const Vec2& b, const Vec2& c, Color color)
    {
        fillPolygon({a, b, c}, color);
    }
    void gradientTriangle(const Vec2& a, Color colorA,
                         const Vec2& b, Color colorB,
                         const Vec2& c, Color colorC)
    {
        if (!ready() || !a.isFinite() || !b.isFinite() || !c.isFinite()) {
            return;
        }
        const float area = edge(a, b, c);
        if (std::abs(area) < 1.0e-8f) {
            return;
        }
        const Rect bounds = Rect{
            std::min({a.x, b.x, c.x}), std::min({a.y, b.y, c.y}),
            std::max({a.x, b.x, c.x}) - std::min({a.x, b.x, c.x}),
            std::max({a.y, b.y, c.y}) - std::min({a.y, b.y, c.y})
        }.intersection(clipBounds());
        for (int y = pixelStart(bounds.top()); y < pixelEnd(bounds.bottom(), height()); ++y) {
            for (int x = pixelStart(bounds.left()); x < pixelEnd(bounds.right(), width()); ++x) {
                const Vec2 p{static_cast<float>(x) + 0.5f,
                             static_cast<float>(y) + 0.5f};
                const float wa = edge(b, c, p) / area;
                const float wb = edge(c, a, p) / area;
                const float wc = 1.0f - wa - wb;
                if (wa >= 0.0f && wb >= 0.0f && wc >= 0.0f) {
                    putPixel(x, y, mix(colorA, colorB, colorC, wa, wb, wc));
                }
            }
        }
    }
    void gradientRect(const Rect& rectangle, Color topLeft, Color topRight,
                      Color bottomRight, Color bottomLeft)
    {
        if (!ready() || !finiteRect(rectangle) || rectangle.isEmpty()) {
            return;
        }
        const Rect bounds = rectangle.normalized().intersection(clipBounds());
        for (int y = pixelStart(bounds.top()); y < pixelEnd(bounds.bottom(), height()); ++y) {
            const float v = std::clamp((static_cast<float>(y) + 0.5f - rectangle.top())
                                       / rectangle.height, 0.0f, 1.0f);
            for (int x = pixelStart(bounds.left()); x < pixelEnd(bounds.right(), width()); ++x) {
                const float u = std::clamp((static_cast<float>(x) + 0.5f - rectangle.left())
                                           / rectangle.width, 0.0f, 1.0f);
                const Color top = Color::lerp(topLeft, topRight, u);
                const Color bottom = Color::lerp(bottomLeft, bottomRight, u);
                putPixel(x, y, Color::lerp(top, bottom, v));
            }
        }
    }
    void linearGradient(const Rect& rectangle, const std::vector<GradientStop2D>& stops,
                        const Vec2& direction)
    {
        if (!ready() || stops.empty() || !finiteRect(rectangle)
            || !direction.isFinite() || direction.lengthSquared() <= 0.0f) {
            return;
        }
        const Rect area = rectangle.normalized().intersection(clipBounds());
        const Vec2 axis = direction.normalized();
        const Vec2 center = rectangle.center();
        const float extent = std::abs(axis.x) * rectangle.width * 0.5f
                           + std::abs(axis.y) * rectangle.height * 0.5f;
        if (extent <= 0.0f) {
            return;
        }
        std::vector<GradientStop2D> ordered = stops;
        std::stable_sort(ordered.begin(), ordered.end(),
            [](const GradientStop2D& lhs, const GradientStop2D& rhs) {
                return lhs.position < rhs.position;
            });
        for (int y = pixelStart(area.top()); y < pixelEnd(area.bottom(), height()); ++y) {
            for (int x = pixelStart(area.left()); x < pixelEnd(area.right(), width()); ++x) {
                const Vec2 point{static_cast<float>(x) + 0.5f,
                                 static_cast<float>(y) + 0.5f};
                const float t = std::clamp((point - center).dot(axis) / (2.0f * extent) + 0.5f,
                                           0.0f, 1.0f);
                putPixel(x, y, evaluateStops(ordered, t));
            }
        }
    }
    void radialGradient(const Vec2& center, float radius,
                        const std::vector<GradientStop2D>& stops)
    {
        if (!ready() || radius <= 0.0f || !std::isfinite(radius) || stops.empty()) {
            return;
        }
        const Rect area = Rect{center.x - radius, center.y - radius,
                               radius * 2.0f, radius * 2.0f}
                              .intersection(clipBounds());
        std::vector<GradientStop2D> ordered = stops;
        std::stable_sort(ordered.begin(), ordered.end(),
            [](const GradientStop2D& lhs, const GradientStop2D& rhs) {
                return lhs.position < rhs.position;
            });
        for (int y = pixelStart(area.top()); y < pixelEnd(area.bottom(), height()); ++y) {
            for (int x = pixelStart(area.left()); x < pixelEnd(area.right(), width()); ++x) {
                const Vec2 p{static_cast<float>(x) + 0.5f,
                             static_cast<float>(y) + 0.5f};
                putPixel(x, y, evaluateStops(ordered,
                    Vec2::distance(center, p) / radius));
            }
        }
    }
    void conicalGradient(const Vec2& center, float startAngle,
                         Color first, Color second, const Rect& bounds)
    {
        if (!ready() || !center.isFinite() || !std::isfinite(startAngle)
            || !finiteRect(bounds)) {
            return;
        }
        constexpr float tau = 6.28318530717958647692f;
        const Rect area = bounds.normalized().intersection(clipBounds());
        for (int y = pixelStart(area.top()); y < pixelEnd(area.bottom(), height()); ++y) {
            for (int x = pixelStart(area.left()); x < pixelEnd(area.right(), width()); ++x) {
                const float angle = std::atan2(static_cast<float>(y) + 0.5f - center.y,
                    static_cast<float>(x) + 0.5f - center.x);
                const float amount = std::fmod(angle - startAngle + tau, tau) / tau;
                putPixel(x, y, Color::lerp(first, second, amount));
            }
        }
    }
    void fillSector(const Vec2& center, const Vec2& radii,
                    float startAngle, float sweepAngle,
                    Color color, unsigned int segments = 48)
    {
        if (!center.isFinite() || !radii.isFinite()
            || radii.x <= 0.0f || radii.y <= 0.0f
            || !std::isfinite(startAngle) || !std::isfinite(sweepAngle)
            || sweepAngle == 0.0f || segments == 0) {
            return;
        }
        segments = std::min(segments, 2048u);
        std::vector<Vec2> points;
        points.reserve(static_cast<std::size_t>(segments) + 2u);
        points.push_back(center);
        for (unsigned int index = 0; index <= segments; ++index) {
            const float t = static_cast<float>(index) / segments;
            const float angle = startAngle + sweepAngle * t;
            points.push_back({center.x + std::cos(angle) * radii.x,
                              center.y + std::sin(angle) * radii.y});
        }
        fillPolygon(points, color);
    }
    void strokeSector(const Vec2& center, const Vec2& radii,
                      float startAngle, float sweepAngle,
                      Color color, float thickness = 1.0f,
                      unsigned int segments = 48)
    {
        if (!center.isFinite() || !radii.isFinite() || segments == 0
            || radii.x <= 0.0f || radii.y <= 0.0f
            || !std::isfinite(startAngle) || !std::isfinite(sweepAngle)) {
            return;
        }
        const Vec2 start{center.x + std::cos(startAngle) * radii.x,
                         center.y + std::sin(startAngle) * radii.y};
        const Vec2 end{center.x + std::cos(startAngle + sweepAngle) * radii.x,
                       center.y + std::sin(startAngle + sweepAngle) * radii.y};
        arc(center, radii, startAngle, sweepAngle, color, thickness, segments);
        line(center, start, color, thickness);
        line(center, end, color, thickness);
    }
    void drawPolarGrid(const Vec2& center, float radius,
                       unsigned int rings, unsigned int spokes,
                       Color color, float thickness = 1.0f)
    {
        if (!center.isFinite() || !std::isfinite(radius) || radius <= 0.0f
            || rings == 0 || spokes == 0) {
            return;
        }
        rings = std::min(rings, 256u);
        spokes = std::min(spokes, 1024u);
        for (unsigned int index = 1; index <= rings; ++index) {
            circle(center, radius * static_cast<float>(index) / rings,
                   color, thickness);
        }
        constexpr float tau = 6.28318530717958647692f;
        for (unsigned int index = 0; index < spokes; ++index) {
            const float angle = tau * static_cast<float>(index) / spokes;
            line(center, {center.x + std::cos(angle) * radius,
                          center.y + std::sin(angle) * radius},
                 color, thickness);
        }
    }
    void heatmap(const Rect& destination, const std::vector<float>& values,
                 unsigned int columns, unsigned int rows,
                 const std::vector<GradientStop2D>& stops)
    {
        if (!ready() || columns == 0 || rows == 0 || stops.empty()
            || static_cast<std::size_t>(columns) * rows != values.size()
            || !finiteRect(destination) || destination.isEmpty()) {
            return;
        }
        std::vector<GradientStop2D> ordered = stops;
        std::stable_sort(ordered.begin(), ordered.end(),
            [](const GradientStop2D& lhs, const GradientStop2D& rhs) {
                return lhs.position < rhs.position;
            });
        const Rect area = destination.normalized().intersection(clipBounds());
        for (int y = pixelStart(area.top()); y < pixelEnd(area.bottom(), height()); ++y) {
            const float v = (static_cast<float>(y) + 0.5f - destination.y)
                          / destination.height;
            const unsigned int row = std::min(rows - 1u,
                static_cast<unsigned int>(std::clamp(v, 0.0f, 1.0f) * rows));
            for (int x = pixelStart(area.left()); x < pixelEnd(area.right(), width()); ++x) {
                const float u = (static_cast<float>(x) + 0.5f - destination.x)
                              / destination.width;
                const unsigned int column = std::min(columns - 1u,
                    static_cast<unsigned int>(std::clamp(u, 0.0f, 1.0f) * columns));
                const float value = values[static_cast<std::size_t>(row) * columns + column];
                if (std::isfinite(value)) {
                    putPixel(x, y, evaluateStops(ordered,
                        std::clamp(value, 0.0f, 1.0f)));
                }
            }
        }
    }

    bool drawImage(const Image& source, const Rect& destination,
                   Sampling2D filter = Sampling2D::Nearest, float opacity = 1.0f)
    {
        if (!ready() || source.empty() || !finiteRect(destination)
            || destination.isEmpty() || !std::isfinite(opacity)) {
            return false;
        }
        Image snapshot;
        const Image* input = &source;
        if (&source == target_) {
            snapshot = source;
            input = &snapshot;
        }
        const Rect area = destination.normalized().intersection(clipBounds());
        for (int y = pixelStart(area.top()); y < pixelEnd(area.bottom(), height()); ++y) {
            const float v = (static_cast<float>(y) + 0.5f - destination.y)
                          / destination.height;
            for (int x = pixelStart(area.left()); x < pixelEnd(area.right(), width()); ++x) {
                const float u = (static_cast<float>(x) + 0.5f - destination.x)
                              / destination.width;
                Color color = sample(*input, {u, v}, filter);
                color.a = static_cast<std::uint8_t>(std::clamp(
                    std::lround(static_cast<float>(color.a) * std::clamp(opacity, 0.0f, 1.0f)),
                    0l, 255l));
                putPixel(x, y, color);
            }
        }
        return true;
    }
    bool drawImageRegion(const Image& source, const Rect& sourceRegion,
                         const Rect& destination, float opacity = 1.0f,
                         Sampling2D filter = Sampling2D::Bilinear)
    {
        if (!ready() || source.empty() || !finiteRect(sourceRegion)
            || !finiteRect(destination) || sourceRegion.isEmpty()
            || destination.isEmpty() || !std::isfinite(opacity)) {
            return false;
        }
        const Image snapshot = (&source == target_) ? Image(source) : Image{};
        const Image& input = (&source == target_) ? snapshot : source;
        const Rect area = destination.normalized().intersection(clipBounds());
        for (int y = pixelStart(area.top()); y < pixelEnd(area.bottom(), height()); ++y) {
            const float v = (static_cast<float>(y) + 0.5f - destination.y)
                          / destination.height;
            for (int x = pixelStart(area.left()); x < pixelEnd(area.right(), width()); ++x) {
                const float u = (static_cast<float>(x) + 0.5f - destination.x)
                              / destination.width;
                const Vec2 uv{
                    (sourceRegion.x + u * sourceRegion.width) / input.width(),
                    (sourceRegion.y + v * sourceRegion.height) / input.height()};
                Color color = sample(input, uv, filter);
                color.a = static_cast<std::uint8_t>(std::lround(
                    color.a * std::clamp(opacity, 0.0f, 1.0f)));
                putPixel(x, y, color);
            }
        }
        return true;
    }
    void texturedTriangle(const Image& texture,
                          const Vec2& a, const Vec2& uvA,
                          const Vec2& b, const Vec2& uvB,
                          const Vec2& c, const Vec2& uvC,
                          Color tint = Color::white())
    {
        if (!ready() || texture.empty() || !a.isFinite() || !b.isFinite()
            || !c.isFinite() || !uvA.isFinite() || !uvB.isFinite()
            || !uvC.isFinite()) {
            return;
        }
        const float area = edge(a, b, c);
        if (std::abs(area) < 1.0e-8f) {
            return;
        }
        const Rect bounds = Rect{
            std::min({a.x, b.x, c.x}), std::min({a.y, b.y, c.y}),
            std::max({a.x, b.x, c.x}) - std::min({a.x, b.x, c.x}),
            std::max({a.y, b.y, c.y}) - std::min({a.y, b.y, c.y})}
            .intersection(clipBounds());
        const Image snapshot = (&texture == target_) ? Image(texture) : Image{};
        const Image& input = (&texture == target_) ? snapshot : texture;
        for (int y = pixelStart(bounds.top()); y < pixelEnd(bounds.bottom(), height()); ++y) {
            for (int x = pixelStart(bounds.left()); x < pixelEnd(bounds.right(), width()); ++x) {
                const Vec2 point{static_cast<float>(x) + 0.5f,
                                 static_cast<float>(y) + 0.5f};
                const float wa = edge(b, c, point) / area;
                const float wb = edge(c, a, point) / area;
                const float wc = 1.0f - wa - wb;
                if (wa < 0.0f || wb < 0.0f || wc < 0.0f) {
                    continue;
                }
                const Vec2 uv = uvA * wa + uvB * wb + uvC * wc;
                putPixel(x, y, Color::multiply(sample(input, uv, sampling_), tint));
            }
        }
    }
    bool drawImage(const ImageL2D& image, const Rect& destination,
                   float opacity = 1.0f)
    {
        return drawImage(image.image(), destination, sampling_, opacity);
    }
    bool drawImageTransformed(const Image& source, const Transform2D& transform,
                              const Vec2& destinationSize,
                              Sampling2D filter = Sampling2D::Bilinear,
                              float opacity = 1.0f)
    {
        if (!ready() || source.empty() || !destinationSize.isFinite()
            || destinationSize.x <= 0.0f || destinationSize.y <= 0.0f) {
            return false;
        }
        const float radius = std::sqrt(destinationSize.lengthSquared()) * 0.5f
                           * std::max(std::abs(transform.scale().x),
                                     std::abs(transform.scale().y));
        const Vec2 center = transform.position();
        const Rect area = Rect{center.x - radius, center.y - radius,
                               radius * 2.0f, radius * 2.0f}
                              .intersection(clipBounds());
        Image snapshot;
        const Image* input = &source;
        if (&source == target_) {
            snapshot = source;
            input = &snapshot;
        }
        for (int y = pixelStart(area.top()); y < pixelEnd(area.bottom(), height()); ++y) {
            for (int x = pixelStart(area.left()); x < pixelEnd(area.right(), width()); ++x) {
                const Vec2 local = transform.inverseTransformPoint(
                    {static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f});
                const Vec2 uv{local.x / destinationSize.x + 0.5f,
                              local.y / destinationSize.y + 0.5f};
                if (uv.x < 0.0f || uv.x >= 1.0f || uv.y < 0.0f || uv.y >= 1.0f) {
                    continue;
                }
                Color color = sample(*input, uv, filter);
                color.a = static_cast<std::uint8_t>(std::clamp(
                    std::lround(static_cast<float>(color.a) * std::clamp(opacity, 0.0f, 1.0f)),
                    0l, 255l));
                putPixel(x, y, color);
            }
        }
        return true;
    }
    bool nineSlice(const Image& source, const Rect& destination,
                  int leftBorder, int topBorder, int rightBorder, int bottomBorder,
                  float opacity = 1.0f)
    {
        if (source.empty() || !finiteRect(destination) || destination.isEmpty()
            || leftBorder < 0 || topBorder < 0 || rightBorder < 0 || bottomBorder < 0
            || leftBorder + rightBorder > source.width()
            || topBorder + bottomBorder > source.height()) {
            return false;
        }
        const float left = std::min(static_cast<float>(leftBorder), destination.width * 0.5f);
        const float right = std::min(static_cast<float>(rightBorder), destination.width * 0.5f);
        const float top = std::min(static_cast<float>(topBorder), destination.height * 0.5f);
        const float bottom = std::min(static_cast<float>(bottomBorder), destination.height * 0.5f);
        const float dx[4]{destination.left(), destination.left() + left,
                          destination.right() - right, destination.right()};
        const float dy[4]{destination.top(), destination.top() + top,
                          destination.bottom() - bottom, destination.bottom()};
        const int sx[4]{0, leftBorder, source.width() - rightBorder, source.width()};
        const int sy[4]{0, topBorder, source.height() - bottomBorder, source.height()};
        for (int row = 0; row < 3; ++row) {
            for (int column = 0; column < 3; ++column) {
                if (dx[column + 1] <= dx[column] || dy[row + 1] <= dy[row]) {
                    continue;
                }
                const Image section = source.cropped(Rect{
                    static_cast<float>(sx[column]), static_cast<float>(sy[row]),
                    static_cast<float>(sx[column + 1] - sx[column]),
                    static_cast<float>(sy[row + 1] - sy[row])});
                drawImage(section, Rect{dx[column], dy[row],
                    dx[column + 1] - dx[column], dy[row + 1] - dy[row]},
                    sampling_, opacity);
            }
        }
        return true;
    }

    void cubicBezier(const Vec2& p0, const Vec2& p1, const Vec2& p2,
                     const Vec2& p3, Color color, float thickness = 1.0f,
                     unsigned int segments = 32)
    {
        if (segments == 0) {
            return;
        }
        std::vector<Vec2> points;
        points.reserve(static_cast<std::size_t>(segments) + 1u);
        for (unsigned int i = 0; i <= segments; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(segments);
            const float u = 1.0f - t;
            points.push_back(p0 * (u * u * u) + p1 * (3.0f * u * u * t)
                + p2 * (3.0f * u * t * t) + p3 * (t * t * t));
        }
        polyline(points, color, thickness);
    }
    void quadraticBezier(const Vec2& p0, const Vec2& p1, const Vec2& p2,
                         Color color, float thickness = 1.0f,
                         unsigned int segments = 24)
    {
        if (segments == 0) {
            return;
        }
        std::vector<Vec2> points;
        points.reserve(static_cast<std::size_t>(segments) + 1u);
        for (unsigned int i = 0; i <= segments; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(segments);
            const float u = 1.0f - t;
            points.push_back(p0 * (u * u) + p1 * (2.0f * u * t) + p2 * (t * t));
        }
        polyline(points, color, thickness);
    }
    void arc(const Vec2& center, const Vec2& radii, float startRadians,
             float sweepRadians, Color color, float thickness = 1.0f,
             unsigned int segments = 32)
    {
        if (segments == 0 || !center.isFinite() || !radii.isFinite()
            || !std::isfinite(startRadians) || !std::isfinite(sweepRadians)) {
            return;
        }
        std::vector<Vec2> points;
        points.reserve(static_cast<std::size_t>(segments) + 1u);
        for (unsigned int i = 0; i <= segments; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(segments);
            const float angle = startRadians + sweepRadians * t;
            points.push_back({center.x + std::cos(angle) * radii.x,
                              center.y + std::sin(angle) * radii.y});
        }
        polyline(points, color, thickness);
    }

    bool floodFill(int startX, int startY, Color replacement,
                   std::size_t maximumPixels = 0)
    {
        if (!ready() || !visible(startX, startY)) {
            return false;
        }
        const Color target = target_->at(startX, startY);
        if (target == replacement) {
            return true;
        }
        std::vector<std::size_t> pending;
        pending.push_back(pixelIndex(startX, startY));
        std::size_t filled = 0;
        while (!pending.empty()) {
            const std::size_t index = pending.back();
            pending.pop_back();
            const int x = static_cast<int>(index % static_cast<std::size_t>(width()));
            const int y = static_cast<int>(index / static_cast<std::size_t>(width()));
            if (!visible(x, y) || target_->at(x, y) != target) {
                continue;
            }
            target_->at(x, y) = replacement;
            if (maximumPixels != 0 && ++filled >= maximumPixels) {
                break;
            }
            if (x > 0) pending.push_back(pixelIndex(x - 1, y));
            if (x + 1 < width()) pending.push_back(pixelIndex(x + 1, y));
            if (y > 0) pending.push_back(pixelIndex(x, y - 1));
            if (y + 1 < height()) pending.push_back(pixelIndex(x, y + 1));
        }
        return true;
    }
    void invert(const Rect& rectangle)
    {
        forEach(rectangle, [](Color& color) { color = color.inverted(); });
    }
    void grayscale(const Rect& rectangle)
    {
        forEach(rectangle, [](Color& color) { color = color.grayscale(); });
    }
    void tint(const Rect& rectangle, Color tintColor)
    {
        forEach(rectangle, [tintColor](Color& color) {
            color = Color::multiply(color, tintColor);
        });
    }
    void opacity(const Rect& rectangle, float amount)
    {
        if (!std::isfinite(amount)) {
            return;
        }
        const float alpha = std::clamp(amount, 0.0f, 1.0f);
        forEach(rectangle, [alpha](Color& color) {
            color.a = static_cast<std::uint8_t>(std::lround(color.a * alpha));
        });
    }
    void boxBlur(const Rect& rectangle, unsigned int radius)
    {
        if (!ready() || radius == 0) {
            return;
        }
        const Rect area = rectangle.normalized().intersection(clipBounds());
        const int left = pixelStart(area.left());
        const int top = pixelStart(area.top());
        const int right = pixelEnd(area.right(), width());
        const int bottom = pixelEnd(area.bottom(), height());
        if (left >= right || top >= bottom) {
            return;
        }
        const Image source = *target_;
        const int r = static_cast<int>(std::min({radius,
            static_cast<unsigned int>(std::max(width(), height())), 64u}));
        for (int y = top; y < bottom; ++y) {
            for (int x = left; x < right; ++x) {
                unsigned int red = 0, green = 0, blue = 0, alpha = 0, count = 0;
                for (int oy = -r; oy <= r; ++oy) {
                    for (int ox = -r; ox <= r; ++ox) {
                        const int sx = x + ox;
                        const int sy = y + oy;
                        if (!source.contains(sx, sy)) {
                            continue;
                        }
                        const Color sample = source.at(sx, sy);
                        red += sample.r; green += sample.g;
                        blue += sample.b; alpha += sample.a; ++count;
                    }
                }
                if (count != 0) {
                    target_->at(x, y) = {
                        static_cast<std::uint8_t>(red / count),
                        static_cast<std::uint8_t>(green / count),
                        static_cast<std::uint8_t>(blue / count),
                        static_cast<std::uint8_t>(alpha / count)};
                }
            }
        }
    }

                void regularPolygon(const Vec2& center, float radius,
                                    unsigned int sides, float rotation,
                                    Color color, bool filled = true,
                                    float thickness = 1.0f)
                {
                    if (!center.isFinite() || !std::isfinite(radius)
                        || !std::isfinite(rotation) || radius <= 0.0f
                        || sides < 3 || sides > 4096) {
                        return;
                    }
                    std::vector<Vec2> points;
                    points.reserve(sides);
                    constexpr float tau = 6.28318530717958647692f;
                    for (unsigned int index = 0; index < sides; ++index) {
                        const float angle = rotation + tau * static_cast<float>(index)
                            / static_cast<float>(sides);
                        points.push_back({center.x + std::cos(angle) * radius,
                                          center.y + std::sin(angle) * radius});
                    }
                    if (filled) {
                        fillPolygon(points, color);
                    } else {
                        polyline(points, color, thickness, true);
                    }
                }
                void star(const Vec2& center, float outerRadius, float innerRadius,
                          unsigned int pointsCount, float rotation,
                          Color color, bool filled = true, float thickness = 1.0f)
                {
                    if (!center.isFinite() || !std::isfinite(outerRadius)
                        || !std::isfinite(innerRadius) || !std::isfinite(rotation)
                        || outerRadius <= 0.0f || innerRadius < 0.0f
                        || innerRadius > outerRadius || pointsCount < 2 || pointsCount > 2048) {
                        return;
                    }
                    std::vector<Vec2> points;
                    points.reserve(static_cast<std::size_t>(pointsCount) * 2u);
                    constexpr float pi = 3.14159265358979323846f;
                    const float step = pi / static_cast<float>(pointsCount);
                    for (unsigned int index = 0; index < pointsCount * 2u; ++index) {
                        const float radius = index % 2u == 0 ? outerRadius : innerRadius;
                        const float angle = rotation - pi * 0.5f + step * static_cast<float>(index);
                        points.push_back({center.x + std::cos(angle) * radius,
                                          center.y + std::sin(angle) * radius});
                    }
                    if (filled) {
                        fillPolygon(points, color);
                    } else {
                        polyline(points, color, thickness, true);
                    }
                }
                void arrow(const Vec2& from, const Vec2& to, Color color,
                           float thickness = 1.0f, float headLength = 12.0f,
                           float headAngle = 0.5235987755982988f)
                {
                    if (!from.isFinite() || !to.isFinite() || headLength <= 0.0f
                        || !std::isfinite(headAngle)) {
                        return;
                    }
                    const Vec2 direction = (to - from).normalized();
                    if (direction.lengthSquared() == 0.0f) {
                        putPixel(static_cast<int>(std::floor(from.x)),
                                 static_cast<int>(std::floor(from.y)), color);
                        return;
                    }
                    const Vec2 wingA = (-direction).rotated(headAngle) * headLength;
                    const Vec2 wingB = (-direction).rotated(-headAngle) * headLength;
                    line(from, to, color, thickness);
                    line(to, to + wingA, color, thickness);
                    line(to, to + wingB, color, thickness);
                }
                void crosshair(const Vec2& center, float armLength,
                               Color color, float thickness = 1.0f)
                {
                    if (armLength < 0.0f || !center.isFinite()) {
                        return;
                    }
                    line({center.x - armLength, center.y}, {center.x + armLength, center.y},
                         color, thickness);
                    line({center.x, center.y - armLength}, {center.x, center.y + armLength},
                         color, thickness);
                }
                void checkerboard(const Rect& rectangle, float cellSize,
                                  Color first, Color second)
                {
                    if (!ready() || !finiteRect(rectangle) || cellSize <= 0.0f
                        || !std::isfinite(cellSize)) {
                        return;
                    }
                    const Rect area = rectangle.normalized().intersection(clipBounds());
                    const int left = pixelStart(area.left());
                    const int top = pixelStart(area.top());
                    const int right = pixelEnd(area.right(), width());
                    const int bottom = pixelEnd(area.bottom(), height());
                    for (int y = top; y < bottom; ++y) {
                        for (int x = left; x < right; ++x) {
                            const int column = static_cast<int>(std::floor(
                                (static_cast<float>(x) - rectangle.x) / cellSize));
                            const int row = static_cast<int>(std::floor(
                                (static_cast<float>(y) - rectangle.y) / cellSize));
                            putPixel(x, y, ((column + row) & 1) == 0 ? first : second);
                        }
                    }
                }
                void hatch(const Rect& rectangle, float spacing, float thickness,
                           float angle, Color color)
                {
                    if (!finiteRect(rectangle) || spacing <= 0.0f || thickness <= 0.0f
                        || !std::isfinite(angle)) {
                        return;
                    }
                    const Rect area = rectangle.normalized().intersection(clipBounds());
                    const Vec2 direction = Vec2::fromAngle(angle);
                    const Vec2 normal = direction.perpendicular();
                    const Vec2 center = rectangle.center();
                    const float extent = std::abs(normal.x) * rectangle.width * 0.5f
                                       + std::abs(normal.y) * rectangle.height * 0.5f;
                    const int first = static_cast<int>(std::floor(-extent / spacing));
                    const int last = static_cast<int>(std::ceil(extent / spacing));
                    for (int index = first; index <= last; ++index) {
                        const Vec2 offset = normal * (static_cast<float>(index) * spacing);
                        const Vec2 start = center + offset - direction * extent * 2.0f;
                        const Vec2 end = center + offset + direction * extent * 2.0f;
                        clipLineToRect(start, end, area, color, thickness);
                    }
                }

                void brightnessContrast(const Rect& rectangle, float brightness,
                                        float contrast)
                {
                    if (!std::isfinite(brightness) || !std::isfinite(contrast)) {
                        return;
                    }
                    const float gain = std::max(0.0f, contrast + 1.0f);
                    forEach(rectangle, [gain, brightness](Color& color) {
                        color.r = channel01((color.r / 255.0f - 0.5f) * gain + 0.5f + brightness);
                        color.g = channel01((color.g / 255.0f - 0.5f) * gain + 0.5f + brightness);
                        color.b = channel01((color.b / 255.0f - 0.5f) * gain + 0.5f + brightness);
                    });
                }
                void exposure(const Rect& rectangle, float stops)
                {
                    if (!std::isfinite(stops)) {
                        return;
                    }
                    const float gain = std::exp2(std::clamp(stops, -16.0f, 16.0f));
                    forEach(rectangle, [gain](Color& color) {
                        color.r = channel01((color.r / 255.0f) * gain);
                        color.g = channel01((color.g / 255.0f) * gain);
                        color.b = channel01((color.b / 255.0f) * gain);
                    });
                }
                void saturation(const Rect& rectangle, float amount)
                {
                    if (!std::isfinite(amount)) {
                        return;
                    }
                    amount = std::max(0.0f, amount);
                    forEach(rectangle, [amount](Color& color) {
                        const float r = color.r / 255.0f;
                        const float g = color.g / 255.0f;
                        const float b = color.b / 255.0f;
                        const float gray = 0.2126f * r + 0.7152f * g + 0.0722f * b;
                        color.r = channel01(gray + (r - gray) * amount);
                        color.g = channel01(gray + (g - gray) * amount);
                        color.b = channel01(gray + (b - gray) * amount);
                    });
                }
                void hueRotate(const Rect& rectangle, float radians)
                {
                    if (!std::isfinite(radians)) {
                        return;
                    }
                    const float cosine = std::cos(radians);
                    const float sine = std::sin(radians);
                    const std::array<std::array<float, 3>, 3> matrix{{
                        {{0.213f + cosine * 0.787f - sine * 0.213f,
                          0.715f - cosine * 0.715f - sine * 0.715f,
                          0.072f - cosine * 0.072f + sine * 0.928f}},
                        {{0.213f - cosine * 0.213f + sine * 0.143f,
                          0.715f + cosine * 0.285f + sine * 0.140f,
                          0.072f - cosine * 0.072f - sine * 0.283f}},
                        {{0.213f - cosine * 0.213f - sine * 0.787f,
                          0.715f - cosine * 0.715f + sine * 0.715f,
                          0.072f + cosine * 0.928f + sine * 0.072f}}
                    }};
                    forEach(rectangle, [&matrix](Color& color) {
                        const float r = color.r / 255.0f;
                        const float g = color.g / 255.0f;
                        const float b = color.b / 255.0f;
                        color.r = channel01(matrix[0][0] * r + matrix[0][1] * g + matrix[0][2] * b);
                        color.g = channel01(matrix[1][0] * r + matrix[1][1] * g + matrix[1][2] * b);
                        color.b = channel01(matrix[2][0] * r + matrix[2][1] * g + matrix[2][2] * b);
                    });
                }
                void gamma(const Rect& rectangle, float value)
                {
                    if (!std::isfinite(value) || value <= 0.0f) {
                        return;
                    }
                    const float exponent = 1.0f / value;
                    forEach(rectangle, [exponent](Color& color) {
                        color.r = channel01(std::pow(color.r / 255.0f, exponent));
                        color.g = channel01(std::pow(color.g / 255.0f, exponent));
                        color.b = channel01(std::pow(color.b / 255.0f, exponent));
                    });
                }
                void threshold(const Rect& rectangle, float luminanceThreshold,
                               Color below, Color above)
                {
                    if (!std::isfinite(luminanceThreshold)) {
                        return;
                    }
                    forEach(rectangle, [luminanceThreshold, below, above](Color& color) {
                        const float luminance = color.luminance();
                        const std::uint8_t alpha = color.a;
                        color = luminance < luminanceThreshold ? below : above;
                        color.a = static_cast<std::uint8_t>(
                            static_cast<unsigned int>(color.a) * alpha / 255u);
                    });
                }
                void posterize(const Rect& rectangle, unsigned int levels)
                {
                    if (levels < 2) {
                        return;
                    }
                    const unsigned int maximum = std::min(levels - 1u, 255u);
                    forEach(rectangle, [maximum](Color& color) {
                        const auto quantize = [maximum](std::uint8_t value) {
                            const unsigned int scaled = static_cast<unsigned int>(value) * maximum;
                            return static_cast<std::uint8_t>(
                                ((scaled + 127u) / 255u) * 255u / maximum);
                        };
                        color.r = quantize(color.r);
                        color.g = quantize(color.g);
                        color.b = quantize(color.b);
                    });
                }
                void solarize(const Rect& rectangle, std::uint8_t thresholdValue)
                {
                    forEach(rectangle, [thresholdValue](Color& color) {
                        if (color.r >= thresholdValue) color.r = 255u - color.r;
                        if (color.g >= thresholdValue) color.g = 255u - color.g;
                        if (color.b >= thresholdValue) color.b = 255u - color.b;
                    });
                }
                void sepia(const Rect& rectangle, float amount = 1.0f)
                {
                    if (!std::isfinite(amount)) {
                        return;
                    }
                    amount = std::clamp(amount, 0.0f, 1.0f);
                    forEach(rectangle, [amount](Color& color) {
                        const float red = color.r;
                        const float green = color.g;
                        const float blue = color.b;
                        const Color sepiaColor{
                            channel255(0.393f * red + 0.769f * green + 0.189f * blue),
                            channel255(0.349f * red + 0.686f * green + 0.168f * blue),
                            channel255(0.272f * red + 0.534f * green + 0.131f * blue),
                            color.a};
                        color = Color::lerp(color, sepiaColor, amount);
                    });
                }
                void colorKey(const Rect& rectangle, Color key, unsigned int tolerance = 0)
                {
                    forEach(rectangle, [key, tolerance](Color& color) {
                        const auto difference = [](std::uint8_t a, std::uint8_t b) {
                            return static_cast<unsigned int>(std::abs(
                                static_cast<int>(a) - static_cast<int>(b)));
                        };
                        if (difference(color.r, key.r) <= tolerance
                            && difference(color.g, key.g) <= tolerance
                            && difference(color.b, key.b) <= tolerance) {
                            color.a = 0;
                        }
                    });
                }
                void replaceColor(const Rect& rectangle, Color from, Color to,
                                  unsigned int tolerance = 0)
                {
                    forEach(rectangle, [from, to, tolerance](Color& color) {
                        const auto difference = [](std::uint8_t a, std::uint8_t b) {
                            return static_cast<unsigned int>(std::abs(
                                static_cast<int>(a) - static_cast<int>(b)));
                        };
                        if (difference(color.r, from.r) <= tolerance
                            && difference(color.g, from.g) <= tolerance
                            && difference(color.b, from.b) <= tolerance) {
                            const std::uint8_t alpha = color.a;
                            color = to;
                            color.a = static_cast<std::uint8_t>(
                                static_cast<unsigned int>(color.a) * alpha / 255u);
                        }
                    });
                }
                void colorMatrix(const Rect& rectangle,
                                 const std::array<float, 20>& matrix)
                {
                    if (!std::all_of(matrix.begin(), matrix.end(),
                                     [](float value) { return std::isfinite(value); })) {
                        return;
                    }
                    forEach(rectangle, [&matrix](Color& color) {
                        const float source[4]{color.r / 255.0f, color.g / 255.0f,
                                              color.b / 255.0f, color.a / 255.0f};
                        float result[4]{};
                        for (std::size_t row = 0; row < 4; ++row) {
                            for (std::size_t column = 0; column < 4; ++column) {
                                result[row] += matrix[row * 5 + column] * source[column];
                            }
                            result[row] += matrix[row * 5 + 4];
                        }
                        color = {channel01(result[0]), channel01(result[1]),
                                 channel01(result[2]), channel01(result[3])};
                    });
                }
                void convolution(const Rect& rectangle, const std::vector<float>& kernel,
                                 unsigned int kernelWidth, unsigned int kernelHeight,
                                 float divisor = 0.0f, float bias = 0.0f,
                                 bool preserveAlpha = true)
                {
                    if (!ready() || !finiteRect(rectangle) || kernelWidth == 0
                        || kernelHeight == 0 || kernelWidth % 2u == 0
                        || kernelHeight % 2u == 0
                        || static_cast<std::size_t>(kernelWidth) * kernelHeight != kernel.size()
                        || !std::all_of(kernel.begin(), kernel.end(),
                                        [](float value) { return std::isfinite(value); })
                        || !std::isfinite(divisor) || !std::isfinite(bias)) {
                        return;
                    }
                    if (divisor == 0.0f) {
                        for (float value : kernel) {
                            divisor += value;
                        }
                        if (std::abs(divisor) < 1.0e-8f) {
                            divisor = 1.0f;
                        }
                    }
                    const Image source = *target_;
                    const Rect area = rectangle.normalized().intersection(clipBounds());
                    const int left = pixelStart(area.left());
                    const int top = pixelStart(area.top());
                    const int right = pixelEnd(area.right(), width());
                    const int bottom = pixelEnd(area.bottom(), height());
                    const int radiusX = static_cast<int>(kernelWidth / 2u);
                    const int radiusY = static_cast<int>(kernelHeight / 2u);
                    for (int y = top; y < bottom; ++y) {
                        for (int x = left; x < right; ++x) {
                            float channels[4]{};
                            std::size_t weightIndex = 0;
                            for (int ky = -radiusY; ky <= radiusY; ++ky) {
                                for (int kx = -radiusX; kx <= radiusX; ++kx, ++weightIndex) {
                                    const Color sample = source.at(
                                        std::clamp(x + kx, 0, width() - 1),
                                        std::clamp(y + ky, 0, height() - 1));
                                    const float weight = kernel[weightIndex];
                                    channels[0] += sample.r * weight;
                                    channels[1] += sample.g * weight;
                                    channels[2] += sample.b * weight;
                                    channels[3] += sample.a * weight;
                                }
                            }
                            const auto convert = [divisor, bias](float value) {
                                return static_cast<std::uint8_t>(std::clamp(
                                    static_cast<int>(std::lround(value / divisor + bias)),
                                    0, 255));
                            };
                            const Color original = source.at(x, y);
                            target_->at(x, y) = {
                                convert(channels[0]), convert(channels[1]),
                                convert(channels[2]),
                                preserveAlpha ? original.a : convert(channels[3])};
                        }
                    }
                }
                void sharpen(const Rect& rectangle, float amount = 1.0f)
                {
                    if (!std::isfinite(amount)) {
                        return;
                    }
                    convolution(rectangle,
                        {0.0f, -amount, 0.0f, -amount, 1.0f + 4.0f * amount,
                         -amount, 0.0f, -amount, 0.0f},
                        3, 3, 1.0f, 0.0f, true);
                }
                void emboss(const Rect& rectangle)
                {
                    convolution(rectangle,
                        {-2.0f, -1.0f, 0.0f, -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 2.0f},
                        3, 3, 1.0f, 128.0f, true);
                }
                void edgeDetect(const Rect& rectangle)
                {
                    convolution(rectangle,
                        {-1.0f, -1.0f, -1.0f, -1.0f, 8.0f, -1.0f, -1.0f, -1.0f, -1.0f},
                        3, 3, 1.0f, 0.0f, true);
                }
                void medianFilter(const Rect& rectangle, unsigned int radius = 1)
                {
                    if (!ready() || radius == 0) {
                        return;
                    }
                    const unsigned int maximumRadius = static_cast<unsigned int>(
                        std::max(width(), height()));
                    const int r = static_cast<int>(std::min({radius, maximumRadius, 64u}));
                    const Image source = *target_;
                    const Rect area = rectangle.normalized().intersection(clipBounds());
                    std::vector<std::uint8_t> red;
                    std::vector<std::uint8_t> green;
                    std::vector<std::uint8_t> blue;
                    std::vector<std::uint8_t> alpha;
                    const std::size_t capacity = static_cast<std::size_t>(r * 2 + 1)
                                               * static_cast<std::size_t>(r * 2 + 1);
                    red.reserve(capacity);
                    green.reserve(capacity);
                    blue.reserve(capacity);
                    alpha.reserve(capacity);
                    for (int y = pixelStart(area.top()); y < pixelEnd(area.bottom(), height()); ++y) {
                        for (int x = pixelStart(area.left()); x < pixelEnd(area.right(), width()); ++x) {
                            red.clear();
                            green.clear();
                            blue.clear();
                            alpha.clear();
                            for (int ky = -r; ky <= r; ++ky) {
                                for (int kx = -r; kx <= r; ++kx) {
                                    const Color sample = source.at(
                                        std::clamp(x + kx, 0, width() - 1),
                                        std::clamp(y + ky, 0, height() - 1));
                                    red.push_back(sample.r);
                                    green.push_back(sample.g);
                                    blue.push_back(sample.b);
                                    alpha.push_back(sample.a);
                                }
                            }
                            const std::size_t middle = red.size() / 2u;
                            const auto select = [middle](std::vector<std::uint8_t>& values) {
                                std::nth_element(values.begin(), values.begin()
                                    + static_cast<std::ptrdiff_t>(middle), values.end());
                                return values[middle];
                            };
                            target_->at(x, y) = {select(red), select(green),
                                                 select(blue), select(alpha)};
                        }
                    }
                }
                void pixelate(const Rect& rectangle, unsigned int blockWidth,
                              unsigned int blockHeight)
                {
                    if (!ready() || blockWidth == 0 || blockHeight == 0) {
                        return;
                    }
                    const Image source = *target_;
                    const Rect area = rectangle.normalized().intersection(clipBounds());
                    const int left = pixelStart(area.left());
                    const int top = pixelStart(area.top());
                    const int right = pixelEnd(area.right(), width());
                    const int bottom = pixelEnd(area.bottom(), height());
                    for (int y = top; y < bottom;) {
                        const int blockBottom = static_cast<int>(std::min<long long>(bottom,
                            static_cast<long long>(y) + blockHeight));
                        for (int x = left; x < right;) {
                            const int blockRight = static_cast<int>(std::min<long long>(right,
                                static_cast<long long>(x) + blockWidth));
                            std::uint64_t red = 0, green = 0, blue = 0, alpha = 0, count = 0;
                            for (int sy = y; sy < blockBottom; ++sy) {
                                for (int sx = x; sx < blockRight; ++sx) {
                                    const Color sample = source.at(sx, sy);
                                    red += sample.r;
                                    green += sample.g;
                                    blue += sample.b;
                                    alpha += sample.a;
                                    ++count;
                                }
                            }
                            if (count != 0) {
                                const Color average{
                                    static_cast<std::uint8_t>(red / count),
                                    static_cast<std::uint8_t>(green / count),
                                    static_cast<std::uint8_t>(blue / count),
                                    static_cast<std::uint8_t>(alpha / count)};
                                for (int sy = y; sy < blockBottom; ++sy) {
                                    for (int sx = x; sx < blockRight; ++sx) {
                                        target_->at(sx, sy) = average;
                                    }
                                }
                            }
                            x = blockRight;
                        }
                        y = blockBottom;
                    }
                }
                void morphologyAlpha(const Rect& rectangle, unsigned int radius,
                                     Morphology2D operation)
                {
                    if (!ready() || radius == 0) {
                        return;
                    }
                    const Image source = *target_;
                    const Rect area = rectangle.normalized().intersection(clipBounds());
                    const int r = static_cast<int>(std::min(radius,
                        static_cast<unsigned int>(std::max(width(), height()))));
                    for (int y = pixelStart(area.top()); y < pixelEnd(area.bottom(), height()); ++y) {
                        for (int x = pixelStart(area.left()); x < pixelEnd(area.right(), width()); ++x) {
                            std::uint8_t alpha = operation == Morphology2D::Dilate ? 0u : 255u;
                            for (int ky = -r; ky <= r; ++ky) {
                                for (int kx = -r; kx <= r; ++kx) {
                                    if (kx * kx + ky * ky > r * r) {
                                        continue;
                                    }
                                    const int sx = x + kx;
                                    const int sy = y + ky;
                                    const std::uint8_t value = source.contains(sx, sy)
                                        ? source.at(sx, sy).a
                                        : static_cast<std::uint8_t>(
                                            operation == Morphology2D::Dilate ? 0 : 255);
                                    alpha = operation == Morphology2D::Dilate
                                        ? std::max(alpha, value) : std::min(alpha, value);
                                }
                            }
                            target_->at(x, y).a = alpha;
                        }
                    }
                }
                void applyLookupTable(const Rect& rectangle,
                                      const std::array<std::uint8_t, 256>& red,
                                      const std::array<std::uint8_t, 256>& green,
                                      const std::array<std::uint8_t, 256>& blue)
                {
                    forEach(rectangle, [&red, &green, &blue](Color& color) {
                        color.r = red[color.r];
                        color.g = green[color.g];
                        color.b = blue[color.b];
                    });
                }
                void premultiplyAlpha(const Rect& rectangle)
                {
                    forEach(rectangle, [](Color& color) {
                        color.r = static_cast<std::uint8_t>(
                            (static_cast<unsigned int>(color.r) * color.a + 127u) / 255u);
                        color.g = static_cast<std::uint8_t>(
                            (static_cast<unsigned int>(color.g) * color.a + 127u) / 255u);
                        color.b = static_cast<std::uint8_t>(
                            (static_cast<unsigned int>(color.b) * color.a + 127u) / 255u);
                    });
                }
                void unpremultiplyAlpha(const Rect& rectangle)
                {
                    forEach(rectangle, [](Color& color) {
                        if (color.a == 0) {
                            color.r = color.g = color.b = 0;
                            return;
                        }
                        const float multiplier = 255.0f / color.a;
                        color.r = channel01(color.r * multiplier / 255.0f);
                        color.g = channel01(color.g * multiplier / 255.0f);
                        color.b = channel01(color.b * multiplier / 255.0f);
                    });
                }
                void replaceAlpha(const Rect& rectangle, std::uint8_t alpha)
                {
                    forEach(rectangle, [alpha](Color& color) { color.a = alpha; });
                }
                void multiplyAlpha(const Rect& rectangle, float amount)
                {
                    if (!std::isfinite(amount)) {
                        return;
                    }
                    amount = std::clamp(amount, 0.0f, 4.0f);
                    forEach(rectangle, [amount](Color& color) {
                        color.a = static_cast<std::uint8_t>(std::clamp(
                            static_cast<int>(std::lround(color.a * amount)), 0, 255));
                    });
                }
                void gaussianBlur(const Rect& rectangle, unsigned int radius,
                                  float sigma = 0.0f)
                {
                    if (!ready() || radius == 0) {
                        return;
                    }
                    radius = std::min(radius, 64u);
                    if (!std::isfinite(sigma) || sigma <= 0.0f) {
                        sigma = std::max(0.5f, static_cast<float>(radius) * 0.5f);
                    }
                    std::vector<float> weights(static_cast<std::size_t>(radius) * 2u + 1u);
                    float total = 0.0f;
                    for (int offset = -static_cast<int>(radius);
                         offset <= static_cast<int>(radius); ++offset) {
                        const float distance = static_cast<float>(offset);
                        const float weight = std::exp(-(distance * distance)
                            / (2.0f * sigma * sigma));
                        weights[static_cast<std::size_t>(offset + static_cast<int>(radius))] = weight;
                        total += weight;
                    }
                    for (float& weight : weights) {
                        weight /= total;
                    }
                    const Image original = *target_;
                    Image horizontal = original;
                    const Rect area = rectangle.normalized().intersection(clipBounds());
                    const int left = pixelStart(area.left());
                    const int top = pixelStart(area.top());
                    const int right = pixelEnd(area.right(), width());
                    const int bottom = pixelEnd(area.bottom(), height());
                    for (int y = top; y < bottom; ++y) {
                        for (int x = left; x < right; ++x) {
                            horizontal.at(x, y) = weightedSample(original, x, y, weights, true);
                        }
                    }
                    for (int y = top; y < bottom; ++y) {
                        for (int x = left; x < right; ++x) {
                            target_->at(x, y) = weightedSample(horizontal, x, y, weights, false);
                        }
                    }
                }
                void applyAlphaMask(const Rect& rectangle, const Image& mask,
                                    const Rect& maskRegion, bool useLuminance = false)
                {
                    if (!ready() || mask.empty() || !finiteRect(rectangle)
                        || !finiteRect(maskRegion) || rectangle.isEmpty()
                        || maskRegion.isEmpty()) {
                        return;
                    }
                    const Image originalMask = (&mask == target_) ? Image(mask) : mask;
                    const Rect area = rectangle.normalized().intersection(clipBounds());
                    for (int y = pixelStart(area.top()); y < pixelEnd(area.bottom(), height()); ++y) {
                        const float v = (static_cast<float>(y) + 0.5f - rectangle.y)
                                      / rectangle.height;
                        for (int x = pixelStart(area.left()); x < pixelEnd(area.right(), width()); ++x) {
                            const float u = (static_cast<float>(x) + 0.5f - rectangle.x)
                                          / rectangle.width;
                            const Vec2 maskUv{
                                (maskRegion.x + u * maskRegion.width) / originalMask.width(),
                                (maskRegion.y + v * maskRegion.height) / originalMask.height()};
                            const Color maskColor = sample(originalMask, maskUv, Sampling2D::Bilinear);
                            const unsigned int maskValue = useLuminance
                                ? static_cast<unsigned int>(std::lround(
                                    maskColor.perceivedBrightness() * 255.0f))
                                : maskColor.a;
                            Color& destination = target_->at(x, y);
                            destination.a = static_cast<std::uint8_t>(
                                static_cast<unsigned int>(destination.a) * maskValue / 255u);
                        }
                    }
                }
                void dropShadow(const Image& source, const Rect& destination,
                                const Vec2& offset, unsigned int blurRadius,
                                Color shadowColor, float opacity = 1.0f)
                {
                    if (!ready() || source.empty() || !finiteRect(destination)
                        || destination.width <= 0.0f || destination.height <= 0.0f
                        || destination.width > static_cast<float>(std::numeric_limits<int>::max())
                        || destination.height > static_cast<float>(std::numeric_limits<int>::max())
                        || !offset.isFinite() || !std::isfinite(opacity)) {
                        return;
                    }
                    Image mask(destination.width > 0.0f && destination.height > 0.0f
                        ? std::max(1, static_cast<int>(std::ceil(destination.width)))
                        : 0,
                        destination.width > 0.0f && destination.height > 0.0f
                        ? std::max(1, static_cast<int>(std::ceil(destination.height)))
                        : 0, Color::transparent());
                    Renderer2D maskRenderer(mask);
                    maskRenderer.setBlendMode(BlendMode2D::Replace);
                    maskRenderer.drawImage(source,
                        {0.0f, 0.0f, static_cast<float>(mask.width()),
                         static_cast<float>(mask.height())});
                    maskRenderer.morphologyAlpha({0.0f, 0.0f,
                        static_cast<float>(mask.width()), static_cast<float>(mask.height())},
                        std::min(blurRadius, 8u), Morphology2D::Dilate);
                    if (blurRadius > 0) {
                        maskRenderer.gaussianBlur({0.0f, 0.0f,
                            static_cast<float>(mask.width()), static_cast<float>(mask.height())},
                            std::min(blurRadius, 32u));
                    }
                    for (Color& pixel : mask.pixels()) {
                        const unsigned int alpha = static_cast<unsigned int>(pixel.a)
                            * shadowColor.a / 255u;
                        pixel = shadowColor.withAlpha(static_cast<std::uint8_t>(
                            std::lround(alpha * std::clamp(opacity, 0.0f, 1.0f))));
                    }
                    drawImage(mask, {destination.x + offset.x, destination.y + offset.y,
                                     destination.width, destination.height});
                }
                void conicGradient(const Vec2& center, float startAngle,
                                   const std::vector<GradientStop2D>& stops,
                                   const Rect& bounds)
                {
                    if (!ready() || !center.isFinite() || !std::isfinite(startAngle)
                        || stops.empty() || !finiteRect(bounds)) {
                        return;
                    }
                    std::vector<GradientStop2D> ordered = stops;
                    std::stable_sort(ordered.begin(), ordered.end(),
                        [](const GradientStop2D& lhs, const GradientStop2D& rhs) {
                            return lhs.position < rhs.position;
                        });
                    const Rect area = bounds.normalized().intersection(clipBounds());
                    constexpr float tau = 6.28318530717958647692f;
                    for (int y = pixelStart(area.top()); y < pixelEnd(area.bottom(), height()); ++y) {
                        for (int x = pixelStart(area.left()); x < pixelEnd(area.right(), width()); ++x) {
                            const float angle = std::atan2(static_cast<float>(y) + 0.5f - center.y,
                                static_cast<float>(x) + 0.5f - center.x) - startAngle;
                            const float t = std::fmod(angle / tau + 1.0f, 1.0f);
                            putPixel(x, y, evaluateStops(ordered, t));
                        }
                    }
                }
                void tileImage(const Image& source, const Rect& destination,
                               const Rect& sourceRegion, const Vec2& tileSize,
                               bool mirrorAlternate = false, float opacity = 1.0f)
                {
                    if (!ready() || source.empty() || !finiteRect(destination)
                        || !finiteRect(sourceRegion) || tileSize.x <= 0.0f
                        || tileSize.y <= 0.0f || destination.isEmpty()) {
                        return;
                    }
                    const Image snapshot = (&source == target_) ? Image(source) : Image{};
                    const Image& input = (&source == target_) ? snapshot : source;
                    const Rect area = destination.normalized().intersection(clipBounds());
                    const float firstRow = std::max(0.0f,
                        std::floor((area.top() - destination.top()) / tileSize.y));
                    const float firstColumn = std::max(0.0f,
                        std::floor((area.left() - destination.left()) / tileSize.x));
                    for (float row = firstRow,
                               y = destination.top() + firstRow * tileSize.y;
                         y < std::min(destination.bottom(), area.bottom());
                         y += tileSize.y, row += 1.0f) {
                        for (float column = firstColumn,
                                   x = destination.left() + firstColumn * tileSize.x;
                             x < std::min(destination.right(), area.right());
                             x += tileSize.x, column += 1.0f) {
                            const float tileWidth = std::min(tileSize.x, destination.right() - x);
                            const float tileHeight = std::min(tileSize.y, destination.bottom() - y);
                            const bool flipX = mirrorAlternate
                                && (static_cast<unsigned long long>(column) & 1ull) != 0ull;
                            const bool flipY = mirrorAlternate
                                && (static_cast<unsigned long long>(row) & 1ull) != 0ull;
                            const Rect clipped = Rect{x, y, tileWidth, tileHeight}.intersection(area);
                            if (clipped.isEmpty()) {
                                continue;
                            }
                            for (int py = pixelStart(clipped.top());
                                 py < pixelEnd(clipped.bottom(), height()); ++py) {
                                for (int px = pixelStart(clipped.left());
                                     px < pixelEnd(clipped.right(), width()); ++px) {
                                    float u = (static_cast<float>(px) + 0.5f - x) / tileSize.x;
                                    float v = (static_cast<float>(py) + 0.5f - y) / tileSize.y;
                                    if (flipX) u = 1.0f - u;
                                    if (flipY) v = 1.0f - v;
                                    const Vec2 uv{
                                        (sourceRegion.x + u * sourceRegion.width) / input.width(),
                                        (sourceRegion.y + v * sourceRegion.height) / input.height()};
                                    Color color = sample(input, uv, sampling_);
                                    color.a = static_cast<std::uint8_t>(std::lround(
                                        color.a * std::clamp(opacity, 0.0f, 1.0f)));
                                    putPixel(px, py, color);
                                }
                            }
                        }
                    }
                }
                bool drawImageQuad(const Image& source, const Vec2& topLeft,
                                   const Vec2& topRight, const Vec2& bottomRight,
                                   const Vec2& bottomLeft, float opacity = 1.0f)
                {
                    if (!ready() || source.empty() || !topLeft.isFinite()
                        || !topRight.isFinite() || !bottomRight.isFinite()
                        || !bottomLeft.isFinite() || !std::isfinite(opacity)) {
                        return false;
                    }
                    const float minX = std::min({topLeft.x, topRight.x, bottomRight.x, bottomLeft.x});
                    const float maxX = std::max({topLeft.x, topRight.x, bottomRight.x, bottomLeft.x});
                    const float minY = std::min({topLeft.y, topRight.y, bottomRight.y, bottomLeft.y});
                    const float maxY = std::max({topLeft.y, topRight.y, bottomRight.y, bottomLeft.y});
                    const Rect area = Rect{minX, minY, maxX - minX, maxY - minY}
                        .intersection(clipBounds());
                    const Image snapshot = (&source == target_) ? Image(source) : Image{};
                    const Image& input = (&source == target_) ? snapshot : source;
                    for (int y = pixelStart(area.top()); y < pixelEnd(area.bottom(), height()); ++y) {
                        for (int x = pixelStart(area.left()); x < pixelEnd(area.right(), width()); ++x) {
                            const Vec2 point{static_cast<float>(x) + 0.5f,
                                             static_cast<float>(y) + 0.5f};
                            Vec2 uv;
                            if (!inverseBilinear(point, topLeft, topRight,
                                                 bottomRight, bottomLeft, uv)) {
                                continue;
                            }
                            if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f) {
                                continue;
                            }
                            Color color = sample(input, uv, sampling_);
                            color.a = static_cast<std::uint8_t>(std::lround(
                                color.a * std::clamp(opacity, 0.0f, 1.0f)));
                            putPixel(x, y, color);
                        }
                    }
                    return true;
                }
                bool resample(const Image& source, Sampling2D filter = Sampling2D::Bilinear)
                {
                    if (!ready() || source.empty()) {
                        return false;
                    }
                    const Image snapshot = (&source == target_) ? Image(source) : Image{};
                    const Image& input = (&source == target_) ? snapshot : source;
                    const Rect area = clipBounds();
                    for (int y = pixelStart(area.top()); y < pixelEnd(area.bottom(), height()); ++y) {
                        for (int x = pixelStart(area.left()); x < pixelEnd(area.right(), width()); ++x) {
                            const Vec2 uv{
                                (static_cast<float>(x) + 0.5f) / width(),
                                (static_cast<float>(y) + 0.5f) / height()};
                            target_->at(x, y) = sample(input, uv, filter);
                        }
                    }
                    return true;
                }
private:
    static std::uint8_t channel01(float value)
    {
        return static_cast<std::uint8_t>(std::clamp(
            static_cast<int>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f)),
            0, 255));
    }
    static std::uint8_t channel255(float value)
    {
        return static_cast<std::uint8_t>(std::clamp(
            static_cast<int>(std::lround(value)), 0, 255));
    }
    static bool finiteRect(const Rect& rectangle)
    {
        return std::isfinite(rectangle.left()) && std::isfinite(rectangle.top())
            && std::isfinite(rectangle.right()) && std::isfinite(rectangle.bottom());
    }
    static Rect segmentBounds(const Vec2& a, const Vec2& b, float radius)
    {
        const float left = std::min(a.x, b.x) - radius;
        const float top = std::min(a.y, b.y) - radius;
        return {left, top, std::abs(a.x - b.x) + radius * 2.0f,
                std::abs(a.y - b.y) + radius * 2.0f};
    }
    static int pixelStart(float edgeValue)
    {
        if (edgeValue <= static_cast<float>(std::numeric_limits<int>::min())) {
            return std::numeric_limits<int>::min();
        }
        if (edgeValue >= static_cast<float>(std::numeric_limits<int>::max())) {
            return std::numeric_limits<int>::max();
        }
        return static_cast<int>(std::floor(edgeValue));
    }
    static int pixelEnd(float edgeValue, int limit)
    {
        if (edgeValue <= 0.0f) {
            return 0;
        }
        if (edgeValue >= static_cast<float>(limit)) {
            return limit;
        }
        return static_cast<int>(std::ceil(edgeValue));
    }
    static float edge(const Vec2& a, const Vec2& b, const Vec2& p)
    {
        return (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x);
    }
    static Color mix(Color a, Color b, Color c, float wa, float wb, float wc)
    {
        const auto channel = [wa, wb, wc](std::uint8_t x, std::uint8_t y,
                                          std::uint8_t z) {
            const float value = static_cast<float>(x) * wa
                              + static_cast<float>(y) * wb
                              + static_cast<float>(z) * wc;
            return static_cast<std::uint8_t>(std::clamp(
                static_cast<int>(std::lround(value)), 0, 255));
        };
        return {channel(a.r, b.r, c.r), channel(a.g, b.g, c.g),
                channel(a.b, b.b, c.b), channel(a.a, b.a, c.a)};
    }
    static Color evaluateStops(const std::vector<GradientStop2D>& stops, float t)
    {
        if (t <= stops.front().position) {
            return stops.front().color;
        }
        for (std::size_t i = 1; i < stops.size(); ++i) {
            if (t <= stops[i].position) {
                const float range = stops[i].position - stops[i - 1].position;
                const float local = range > 0.0f
                    ? (t - stops[i - 1].position) / range : 1.0f;
                return Color::lerp(stops[i - 1].color, stops[i].color, local);
            }
        }
        return stops.back().color;
    }
    static Color sample(const Image& image, const Vec2& uv, Sampling2D filter)
    {
        if (filter == Sampling2D::Nearest) {
            return image.sampleNearest(uv);
        }
        const float px = std::clamp(uv.x, 0.0f, 1.0f) * image.width() - 0.5f;
        const float py = std::clamp(uv.y, 0.0f, 1.0f) * image.height() - 0.5f;
        const int x0 = static_cast<int>(std::floor(px));
        const int y0 = static_cast<int>(std::floor(py));
        const float tx = px - x0;
        const float ty = py - y0;
        const auto get = [&image](int x, int y) {
            return image.at(std::clamp(x, 0, image.width() - 1),
                            std::clamp(y, 0, image.height() - 1));
        };
        return Color::lerp(Color::lerp(get(x0, y0), get(x0 + 1, y0), tx),
                           Color::lerp(get(x0, y0 + 1), get(x0 + 1, y0 + 1), tx),
                           ty);
    }
    static Color weightedSample(const Image& source, int x, int y,
                                const std::vector<float>& weights,
                                bool horizontal)
    {
        const int radius = static_cast<int>(weights.size() / 2u);
        float channels[4]{};
        for (int offset = -radius; offset <= radius; ++offset) {
            const int sx = std::clamp(x + (horizontal ? offset : 0),
                                      0, source.width() - 1);
            const int sy = std::clamp(y + (horizontal ? 0 : offset),
                                      0, source.height() - 1);
            const Color color = source.at(sx, sy);
            const float weight = weights[static_cast<std::size_t>(offset + radius)];
            channels[0] += color.r * weight;
            channels[1] += color.g * weight;
            channels[2] += color.b * weight;
            channels[3] += color.a * weight;
        }
        const auto convert = [](float value) {
            return static_cast<std::uint8_t>(std::clamp(
                static_cast<int>(std::lround(value)), 0, 255));
        };
        return {convert(channels[0]), convert(channels[1]),
                convert(channels[2]), convert(channels[3])};
    }
    static bool inverseBilinear(const Vec2& point,
                               const Vec2& topLeft,
                               const Vec2& topRight,
                               const Vec2& bottomRight,
                               const Vec2& bottomLeft,
                               Vec2& uv)
    {
        const Vec2 horizontal = topRight - topLeft;
        const Vec2 vertical = bottomLeft - topLeft;
        const Vec2 bilinear = topLeft - topRight + bottomRight - bottomLeft;
        uv = {0.5f, 0.5f};
        for (unsigned int iteration = 0; iteration < 16; ++iteration) {
            const Vec2 mapped = topLeft + horizontal * uv.x + vertical * uv.y
                              + bilinear * (uv.x * uv.y);
            const Vec2 error = mapped - point;
            const Vec2 derivativeU = horizontal + bilinear * uv.y;
            const Vec2 derivativeV = vertical + bilinear * uv.x;
            const float determinant = derivativeU.cross(derivativeV);
            if (std::abs(determinant) < 1.0e-10f) {
                return false;
            }
            const float deltaU = error.cross(derivativeV) / determinant;
            const float deltaV = derivativeU.cross(error) / determinant;
            uv.x -= deltaU;
            uv.y -= deltaV;
            if (!uv.isFinite()) {
                return false;
            }
            if (deltaU * deltaU + deltaV * deltaV < 1.0e-10f) {
                break;
            }
        }
        return true;
    }
    static void appendArc(std::vector<Vec2>& points, const Vec2& center,
                          float radius, float start, float end,
                          unsigned int segments)
    {
        for (unsigned int i = 0; i <= segments; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(segments);
            const float angle = start + (end - start) * t;
            points.push_back({center.x + std::cos(angle) * radius,
                              center.y + std::sin(angle) * radius});
        }
    }
    void clipLineToRect(Vec2 from, Vec2 to, const Rect& clip,
                        Color color, float thickness)
    {
        const Vec2 delta = to - from;
        float first = 0.0f;
        float last = 1.0f;
        const auto clipEdge = [&first, &last](float p, float q) {
            if (p == 0.0f) {
                return q >= 0.0f;
            }
            const float ratio = q / p;
            if (p < 0.0f) {
                if (ratio > last) {
                    return false;
                }
                first = std::max(first, ratio);
            } else {
                if (ratio < first) {
                    return false;
                }
                last = std::min(last, ratio);
            }
            return true;
        };
        if (!clipEdge(-delta.x, from.x - clip.left())
            || !clipEdge(delta.x, clip.right() - from.x)
            || !clipEdge(-delta.y, from.y - clip.top())
            || !clipEdge(delta.y, clip.bottom() - from.y)
            || first > last) {
            return;
        }
        const Vec2 start = from + delta * first;
        const Vec2 end = from + delta * last;
        const std::size_t previousDepth = clips_.size();
        clips_.push_back(clip);
        line(start, end, color, thickness);
        clips_.resize(previousDepth);
    }
    bool visible(int x, int y) const
    {
        return ready() && target_->contains(x, y) && clipBounds().contains(
            {static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f});
    }
    std::size_t pixelIndex(int x, int y) const
    {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(width())
             + static_cast<std::size_t>(x);
    }
    void blend(Color& destination, Color source) const
    {
        switch (blendMode_) {
        case BlendMode2D::Alpha:
            destination = Color::alphaBlend(source, destination);
            break;
        case BlendMode2D::Replace:
            destination = source;
            break;
        case BlendMode2D::Add:
            destination = {
                static_cast<std::uint8_t>(std::min(255, destination.r + source.r * source.a / 255)),
                static_cast<std::uint8_t>(std::min(255, destination.g + source.g * source.a / 255)),
                static_cast<std::uint8_t>(std::min(255, destination.b + source.b * source.a / 255)),
                std::max(destination.a, source.a)};
            break;
        case BlendMode2D::Multiply: {
            const Color multiplied = Color::multiply(destination, source);
            const float amount = static_cast<float>(source.a) / 255.0f;
            destination = Color::lerp(destination, multiplied, amount)
                .withAlpha(std::max(destination.a, source.a));
            break;
        }
        case BlendMode2D::Screen: {
            const Color screened{
                static_cast<std::uint8_t>(255 - (255 - destination.r) * (255 - source.r) / 255),
                static_cast<std::uint8_t>(255 - (255 - destination.g) * (255 - source.g) / 255),
                static_cast<std::uint8_t>(255 - (255 - destination.b) * (255 - source.b) / 255),
                std::max(destination.a, source.a)};
            destination = Color::lerp(destination, screened,
                static_cast<float>(source.a) / 255.0f).withAlpha(screened.a);
            break;
        }
        }
    }
    template<class Function>
    void forEach(const Rect& rectangle, Function function)
    {
        if (!ready() || !finiteRect(rectangle)) {
            return;
        }
        const Rect area = rectangle.normalized().intersection(clipBounds());
        for (int y = pixelStart(area.top()); y < pixelEnd(area.bottom(), height()); ++y) {
            for (int x = pixelStart(area.left()); x < pixelEnd(area.right(), width()); ++x) {
                function(target_->at(x, y));
            }
        }
    }

    Image* target_ = nullptr;
    BlendMode2D blendMode_ = BlendMode2D::Alpha;
    Sampling2D sampling_ = Sampling2D::Bilinear;
    std::vector<Rect> clips_;
};

using Renderer2DR = Renderer2D;

} // namespace peco
