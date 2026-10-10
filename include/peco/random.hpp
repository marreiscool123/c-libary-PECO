#pragma once

#include <cstdint>
#include <cmath>
#include <iterator>
#include <random>
#include <stdexcept>
#include <type_traits>

#include <peco\vector2.hpp>
#include <peco\vector3.hpp>

namespace peco {

class Random {
public:
    using Engine = std::mt19937;

    Random() : engine_(std::random_device{}()) {}
    explicit Random(std::uint32_t seed) : engine_(seed) {}

    void seed(std::uint32_t value) { engine_.seed(value); }
    std::uint32_t nextUInt()
    {
        return std::uniform_int_distribution<std::uint32_t>{} (engine_);
    }
    int integer(int minimum, int maximum)
    {
        if (minimum > maximum) {
            throw std::invalid_argument("Peco random integer bounds are reversed.");
        }
        return std::uniform_int_distribution<int>(minimum, maximum)(engine_);
    }
    unsigned int integer(unsigned int minimum, unsigned int maximum)
    {
        if (minimum > maximum) {
            throw std::invalid_argument("Peco random integer bounds are reversed.");
        }
        return std::uniform_int_distribution<unsigned int>(minimum, maximum)(engine_);
    }
    float real(float minimum = 0.0f, float maximum = 1.0f)
    {
        if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum > maximum) {
            throw std::invalid_argument("Peco random real bounds must be finite and ordered.");
        }
        return std::uniform_real_distribution<float>(minimum, maximum)(engine_);
    }
    double real(double minimum = 0.0, double maximum = 1.0)
    {
        if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum > maximum) {
            throw std::invalid_argument("Peco random real bounds must be finite and ordered.");
        }
        return std::uniform_real_distribution<double>(minimum, maximum)(engine_);
    }
    bool boolean(float probability = 0.5f)
    {
        if (!std::isfinite(probability)) {
            throw std::invalid_argument("Peco random probability must be finite.");
        }
        probability = probability < 0.0f ? 0.0f : (probability > 1.0f ? 1.0f : probability);
        return real(0.0f, 1.0f) < probability;
    }
    Vec2 pointInUnitCircle()
    {
        const float angle = real(0.0f, 6.28318530717958647692f);
        const float radius = std::sqrt(real(0.0f, 1.0f));
        return {std::cos(angle) * radius, std::sin(angle) * radius};
    }
    Vec2 pointOnUnitCircle()
    {
        const float angle = real(0.0f, 6.28318530717958647692f);
        return {std::cos(angle), std::sin(angle)};
    }
    Vec3 pointInUnitSphere()
    {
        const float z = real(-1.0f, 1.0f);
        const float angle = real(0.0f, 6.28318530717958647692f);
        const float radial = std::sqrt(1.0f - z * z);
        const float distance = std::cbrt(real(0.0f, 1.0f));
        return {distance * radial * std::cos(angle),
                distance * radial * std::sin(angle), distance * z};
    }
    Vec3 pointOnUnitSphere()
    {
        const float z = real(-1.0f, 1.0f);
        const float angle = real(0.0f, 6.28318530717958647692f);
        const float radial = std::sqrt(1.0f - z * z);
        return {radial * std::cos(angle), radial * std::sin(angle), z};
    }
    template<class Iterator>
    void shuffle(Iterator first, Iterator last)
    {
        std::shuffle(first, last, engine_);
    }
    template<class Container>
    auto choose(const Container& values) -> decltype(values.begin())
    {
        if (values.begin() == values.end()) {
            return values.end();
        }
        const auto count = std::distance(values.begin(), values.end());
        auto selected = values.begin();
        std::advance(selected, integer(0, static_cast<int>(count - 1)));
        return selected;
    }
    Engine& engine() { return engine_; }
    const Engine& engine() const { return engine_; }

private:
    Engine engine_;
};

} // namespace peco
