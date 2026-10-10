#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <vector>

#include <peco\color.hpp>
#include <peco\rect.hpp>

namespace peco {

class Image {
public:
    Image() = default;
    Image(int width, int height, Color fill = Color::transparent())
    {
        if (!create(width, height, fill)) {
            throw std::invalid_argument("Peco image dimensions are invalid or too large.");
        }
    }

    bool create(int width, int height, Color fill = Color::transparent())
    {
        if (width <= 0 || height <= 0) {
            return false;
        }
        const std::size_t w = static_cast<std::size_t>(width);
        const std::size_t h = static_cast<std::size_t>(height);
        if (w > std::numeric_limits<std::size_t>::max() / h) {
            return false;
        }
        width_ = width;
        height_ = height;
        pixels_.assign(w * h, fill);
        return true;
    }
    void clear(Color color = Color::transparent())
    {
        std::fill(pixels_.begin(), pixels_.end(), color);
    }
    int width() const { return width_; }
    int height() const { return height_; }
    bool empty() const { return pixels_.empty(); }
    std::size_t pixelCount() const { return pixels_.size(); }
    const std::vector<Color>& pixels() const { return pixels_; }
    std::vector<Color>& pixels() { return pixels_; }
    const Color* data() const { return pixels_.data(); }
    Color* data() { return pixels_.data(); }
    bool contains(int x, int y) const
    {
        return x >= 0 && y >= 0 && x < width_ && y < height_;
    }
    bool getPixel(int x, int y, Color& result) const
    {
        if (!contains(x, y)) {
            return false;
        }
        result = pixels_[indexOf(x, y)];
        return true;
    }
    bool setPixel(int x, int y, Color color)
    {
        if (!contains(x, y)) {
            return false;
        }
        pixels_[indexOf(x, y)] = color;
        return true;
    }
    Color& at(int x, int y)
    {
        if (!contains(x, y)) {
            throw std::out_of_range("Peco image pixel is outside the image bounds.");
        }
        return pixels_[indexOf(x, y)];
    }
    const Color& at(int x, int y) const
    {
        if (!contains(x, y)) {
            throw std::out_of_range("Peco image pixel is outside the image bounds.");
        }
        return pixels_[indexOf(x, y)];
    }
    Color sampleNearest(const Vec2& uv, bool repeat = false) const
    {
        if (empty() || !uv.isFinite()) {
            return Color::transparent();
        }
        float u = uv.x;
        float v = uv.y;
        if (repeat) {
            u -= std::floor(u);
            v -= std::floor(v);
        } else {
            u = std::clamp(u, 0.0f, 1.0f);
            v = std::clamp(v, 0.0f, 1.0f);
        }
        const int x = std::clamp(static_cast<int>(u * static_cast<float>(width_)),
                                 0, width_ - 1);
        const int y = std::clamp(static_cast<int>(v * static_cast<float>(height_)),
                                 0, height_ - 1);
        return at(x, y);
    }
    void fillRect(const Rect& rectangle, Color color)
    {
        if (empty() || !std::isfinite(rectangle.left()) || !std::isfinite(rectangle.top())
            || !std::isfinite(rectangle.right()) || !std::isfinite(rectangle.bottom())) {
            return;
        }
        const int left = static_cast<int>(std::floor(std::clamp(
            rectangle.left(), 0.0f, static_cast<float>(width_))));
        const int top = static_cast<int>(std::floor(std::clamp(
            rectangle.top(), 0.0f, static_cast<float>(height_))));
        const int right = static_cast<int>(std::ceil(std::clamp(
            rectangle.right(), 0.0f, static_cast<float>(width_))));
        const int bottom = static_cast<int>(std::ceil(std::clamp(
            rectangle.bottom(), 0.0f, static_cast<float>(height_))));
        if (right <= left || bottom <= top) {
            return;
        }
        for (int y = top; y < bottom; ++y) {
            auto first = pixels_.begin() + static_cast<std::ptrdiff_t>(indexOf(left, y));
            std::fill(first, first + (right - left), color);
        }
    }
    void drawLine(int x0, int y0, int x1, int y1, Color color, int thickness = 1)
    {
        if (empty()) {
            return;
        }
        const double startX = x0;
        const double startY = y0;
        const double deltaX = static_cast<double>(x1) - startX;
        const double deltaY = static_cast<double>(y1) - startY;
        double first = 0.0;
        double last = 1.0;
        const auto clip = [&first, &last](double p, double q) {
            if (p == 0.0) {
                return q >= 0.0;
            }
            const double ratio = q / p;
            if (p < 0.0) {
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
        if (!clip(-deltaX, startX)
            || !clip(deltaX, static_cast<double>(width_ - 1) - startX)
            || !clip(-deltaY, startY)
            || !clip(deltaY, static_cast<double>(height_ - 1) - startY)
            || first > last) {
            return;
        }
        x0 = static_cast<int>(std::lround(startX + first * deltaX));
        y0 = static_cast<int>(std::lround(startY + first * deltaY));
        x1 = static_cast<int>(std::lround(startX + last * deltaX));
        y1 = static_cast<int>(std::lround(startY + last * deltaY));
        const int brushRadius = std::max(0, thickness - 1) / 2;
        const long long dx = std::llabs(static_cast<long long>(x1) - x0);
        const int sx = x0 < x1 ? 1 : -1;
        const long long dy = -std::llabs(static_cast<long long>(y1) - y0);
        const int sy = y0 < y1 ? 1 : -1;
        long long error = dx + dy;
        for (;;) {
            fillRect({static_cast<float>(x0) - static_cast<float>(brushRadius),
                      static_cast<float>(y0) - static_cast<float>(brushRadius),
                      static_cast<float>(std::max(1, thickness)),
                      static_cast<float>(std::max(1, thickness))}, color);
            if (x0 == x1 && y0 == y1) {
                break;
            }
            const long long twiceError = error * 2;
            if (twiceError >= dy) {
                error += dy;
                x0 += sx;
            }
            if (twiceError <= dx) {
                error += dx;
                y0 += sy;
            }
        }
    }
    void fillCircle(int centerX, int centerY, int radius, Color color)
    {
        if (radius < 0 || empty()) {
            return;
        }
        const long long minimumY = std::max(0LL, static_cast<long long>(centerY) - radius);
        const long long maximumY = std::min(static_cast<long long>(height_ - 1),
                                             static_cast<long long>(centerY) + radius);
        const long long radiusSquared = static_cast<long long>(radius) * radius;
        for (long long y = minimumY; y <= maximumY; ++y) {
            const long long dy = y - centerY;
            const int extent = static_cast<int>(std::floor(std::sqrt(
                static_cast<double>(radiusSquared - dy * dy))));
            fillRect({static_cast<float>(static_cast<long long>(centerX) - extent),
                      static_cast<float>(y),
                      static_cast<float>(extent) * 2.0f + 1.0f, 1.0f}, color);
        }
    }
    bool blit(const Image& source, int destinationX, int destinationY,
              float opacity = 1.0f)
    {
        if (source.empty() || empty()) {
            return false;
        }
        Image sourceSnapshot;
        const Image* input = &source;
        if (this == &source) {
            sourceSnapshot = source;
            input = &sourceSnapshot;
        }
        const int left = std::max(0, destinationX);
        const int top = std::max(0, destinationY);
        const int right = static_cast<int>(std::min(
            static_cast<long long>(width_),
            static_cast<long long>(destinationX) + input->width_));
        const int bottom = static_cast<int>(std::min(
            static_cast<long long>(height_),
            static_cast<long long>(destinationY) + input->height_));
        if (left >= right || top >= bottom) {
            return false;
        }
        const float alphaScale = std::clamp(opacity, 0.0f, 1.0f);
        for (int y = top; y < bottom; ++y) {
            for (int x = left; x < right; ++x) {
                const int sourceX = static_cast<int>(
                    static_cast<long long>(x) - destinationX);
                const int sourceY = static_cast<int>(
                    static_cast<long long>(y) - destinationY);
                Color foreground = input->at(sourceX, sourceY);
                foreground.a = static_cast<std::uint8_t>(
                    std::lround(static_cast<float>(foreground.a) * alphaScale));
                at(x, y) = Color::alphaBlend(foreground, at(x, y));
            }
        }
        return true;
    }
    Image flippedHorizontally() const
    {
        if (empty()) {
            return {};
        }
        Image result(width_, height_);
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                result.at(width_ - 1 - x, y) = at(x, y);
            }
        }
        return result;
    }
    Image flippedVertically() const
    {
        if (empty()) {
            return {};
        }
        Image result(width_, height_);
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                result.at(x, height_ - 1 - y) = at(x, y);
            }
        }
        return result;
    }
    Image cropped(const Rect& rectangle) const
    {
        if (empty() || !std::isfinite(rectangle.left()) || !std::isfinite(rectangle.top())
            || !std::isfinite(rectangle.right()) || !std::isfinite(rectangle.bottom())) {
            return {};
        }
        const int left = static_cast<int>(std::floor(std::clamp(
            rectangle.left(), 0.0f, static_cast<float>(width_))));
        const int top = static_cast<int>(std::floor(std::clamp(
            rectangle.top(), 0.0f, static_cast<float>(height_))));
        const int right = static_cast<int>(std::ceil(std::clamp(
            rectangle.right(), 0.0f, static_cast<float>(width_))));
        const int bottom = static_cast<int>(std::ceil(std::clamp(
            rectangle.bottom(), 0.0f, static_cast<float>(height_))));
        if (right <= left || bottom <= top) {
            return {};
        }
        Image result(right - left, bottom - top);
        for (int y = top; y < bottom; ++y) {
            for (int x = left; x < right; ++x) {
                result.at(x - left, y - top) = at(x, y);
            }
        }
        return result;
    }

private:
    std::size_t indexOf(int x, int y) const
    {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(width_)
            + static_cast<std::size_t>(x);
    }

    int width_ = 0;
    int height_ = 0;
    std::vector<Color> pixels_;
};

} // namespace peco
