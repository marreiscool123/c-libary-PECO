#pragma once

#include <string>
#include <cstddef>
#include <vector>

#include <peco\color.hpp>
#include <peco\image.hpp>
#include <peco\platform.hpp>
#include <peco\path2d.hpp>

namespace peco {

class Canvas2D {
public:
    enum class TextAlignment {
        Left,
        Center,
        Right
    };

    explicit Canvas2D(HDC deviceContext) : deviceContext_(deviceContext) {}
    ~Canvas2D();
    Canvas2D(const Canvas2D&) = delete;
    Canvas2D& operator=(const Canvas2D&) = delete;

    void clear(Color color);
    void line(int x1, int y1, int x2, int y2, Color color, int thickness = 1);
    void rectangle(int x, int y, int width, int height, Color color, int thickness = 1);
    void fillRectangle(int x, int y, int width, int height, Color color);
    void circle(int centerX, int centerY, int radius, Color color, int thickness = 1);
    void fillCircle(int centerX, int centerY, int radius, Color color);
    void text(int x, int y, const std::wstring& value, Color color, int height = 20);
    void polyline(const Vec2* points, std::size_t count,
                  Color color, int thickness = 1, bool closed = false);
    void polyline(const std::vector<Vec2>& points, Color color,
                  int thickness = 1, bool closed = false);
    void fillPolygon(const Vec2* points, std::size_t count, Color color);
    void fillPolygon(const std::vector<Vec2>& points, Color color);
    bool drawPath(const Path2D& path, Color color, int thickness = 1,
                  float tolerance = 0.5f);
    bool fillPath(const Path2D& path, Color color, float tolerance = 0.5f);
    bool pushClip(const Rect& rectangle);
    bool pushState();
    bool popState();
    void setTextAlignment(TextAlignment alignment);
    void setTextBackgroundTransparent(bool transparent);
    bool drawPixel(int x, int y, Color color);
    bool drawImage(const Image& image, const Rect& destination,
                   float opacity = 1.0f);
    HDC deviceContext() const { return deviceContext_; }

private:
    bool buildPath(const Path2D& path, float tolerance);
    HDC deviceContext_;
    std::vector<int> savedStates_;
};

} // namespace peco
