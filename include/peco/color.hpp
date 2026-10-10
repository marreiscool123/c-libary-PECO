#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace peco {

struct Color {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;

    constexpr Color() = default;
    constexpr Color(std::uint8_t red, std::uint8_t green,
                    std::uint8_t blue, std::uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}

    constexpr bool operator==(const Color& rhs) const
    {
        return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
    }
    constexpr bool operator!=(const Color& rhs) const { return !(*this == rhs); }
    constexpr std::uint32_t rgba() const
    {
        return (static_cast<std::uint32_t>(r) << 24)
             | (static_cast<std::uint32_t>(g) << 16)
             | (static_cast<std::uint32_t>(b) << 8)
             | static_cast<std::uint32_t>(a);
    }
    constexpr std::uint32_t argb() const
    {
        return (static_cast<std::uint32_t>(a) << 24)
             | (static_cast<std::uint32_t>(r) << 16)
             | (static_cast<std::uint32_t>(g) << 8)
             | static_cast<std::uint32_t>(b);
    }
    static constexpr Color fromRGBA(std::uint32_t value)
    {
        return {static_cast<std::uint8_t>(value >> 24),
                static_cast<std::uint8_t>(value >> 16),
                static_cast<std::uint8_t>(value >> 8),
                static_cast<std::uint8_t>(value)};
    }
    static constexpr Color fromARGB(std::uint32_t value)
    {
        return {static_cast<std::uint8_t>(value >> 16),
                static_cast<std::uint8_t>(value >> 8),
                static_cast<std::uint8_t>(value),
                static_cast<std::uint8_t>(value >> 24)};
    }
    constexpr Color withAlpha(std::uint8_t alpha) const { return {r, g, b, alpha}; }

    static Color lerp(Color from, Color to, float amount)
    {
        const float t = std::clamp(amount, 0.0f, 1.0f);
        const auto channel = [t](std::uint8_t start, std::uint8_t end) {
            return static_cast<std::uint8_t>(std::clamp(
                static_cast<int>(std::lround(start + (end - start) * t)), 0, 255));
        };
        return {channel(from.r, to.r), channel(from.g, to.g),
                channel(from.b, to.b), channel(from.a, to.a)};
    }
    static Color multiply(Color lhs, Color rhs)
    {
        const auto channel = [](std::uint8_t a, std::uint8_t b) {
            return static_cast<std::uint8_t>((static_cast<unsigned int>(a) * b + 127u) / 255u);
        };
        return {channel(lhs.r, rhs.r), channel(lhs.g, rhs.g),
                channel(lhs.b, rhs.b), channel(lhs.a, rhs.a)};
    }
    static Color alphaBlend(Color foreground, Color background)
    {
        const unsigned int alpha = foreground.a;
        const unsigned int inverse = 255u - alpha;
        const unsigned int outputAlpha = alpha
            + (static_cast<unsigned int>(background.a) * inverse + 127u) / 255u;
        if (outputAlpha == 0u) {
            return transparent();
        }
        const auto channel = [alpha, inverse, &background, outputAlpha](
            std::uint8_t front, std::uint8_t back) {
            const unsigned int backgroundContribution =
                (static_cast<unsigned int>(back) * background.a * inverse + 127u) / 255u;
            const unsigned int numerator =
                static_cast<unsigned int>(front) * alpha + backgroundContribution;
            return static_cast<std::uint8_t>(
                std::min(255u, (numerator + outputAlpha / 2u) / outputAlpha));
        };
        return {channel(foreground.r, background.r),
                channel(foreground.g, background.g),
                channel(foreground.b, background.b),
                static_cast<std::uint8_t>(outputAlpha)};
    }
    float luminance() const
    {
        const auto linearize = [](std::uint8_t component) {
            const float value = static_cast<float>(component) / 255.0f;
            return value <= 0.04045f ? value / 12.92f
                : std::pow((value + 0.055f) / 1.055f, 2.4f);
        };
        return 0.2126f * linearize(r) + 0.7152f * linearize(g) + 0.0722f * linearize(b);
    }
    float perceivedBrightness() const
    {
        return (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;
    }
    Color inverted() const { return {static_cast<std::uint8_t>(255u - r),
                                     static_cast<std::uint8_t>(255u - g),
                                     static_cast<std::uint8_t>(255u - b), a}; }
    Color grayscale() const
    {
        const auto value = static_cast<std::uint8_t>(std::clamp(
            static_cast<int>(std::lround(perceivedBrightness() * 255.0f)), 0, 255));
        return {value, value, value, a};
    }

    static constexpr Color transparent() { return {0, 0, 0, 0}; }
    static constexpr Color black() { return {0, 0, 0, 255}; }
    static constexpr Color white() { return {255, 255, 255, 255}; }
    static constexpr Color red() { return {255, 0, 0, 255}; }
    static constexpr Color green() { return {0, 255, 0, 255}; }
    static constexpr Color blue() { return {0, 0, 255, 255}; }
    static constexpr Color yellow() { return {255, 255, 0, 255}; }
    static constexpr Color cyan() { return {0, 255, 255, 255}; }
    static constexpr Color magenta() { return {255, 0, 255, 255}; }
    static constexpr Color gray(std::uint8_t shade) { return {shade, shade, shade, 255}; }
};

} // namespace peco
