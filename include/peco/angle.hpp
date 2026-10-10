#pragma once

#include <cmath>

namespace peco {

class Angle {
public:
    constexpr Angle() = default;
    static constexpr Angle radians(float value) { return Angle(value); }
    static Angle degrees(float value)
    {
        return Angle(value * 0.01745329251994329577f);
    }

    constexpr float asRadians() const { return radians_; }
    constexpr float asDegrees() const { return radians_ * 57.295779513082320876f; }
    constexpr Angle operator+(Angle rhs) const { return Angle(radians_ + rhs.radians_); }
    constexpr Angle operator-(Angle rhs) const { return Angle(radians_ - rhs.radians_); }
    constexpr Angle operator*(float scalar) const { return Angle(radians_ * scalar); }
    constexpr Angle operator/(float scalar) const { return Angle(radians_ / scalar); }
    constexpr Angle& operator+=(Angle rhs) { radians_ += rhs.radians_; return *this; }
    constexpr Angle& operator-=(Angle rhs) { radians_ -= rhs.radians_; return *this; }
    constexpr bool operator==(Angle rhs) const { return radians_ == rhs.radians_; }
    constexpr bool operator!=(Angle rhs) const { return !(*this == rhs); }
    constexpr bool operator<(Angle rhs) const { return radians_ < rhs.radians_; }
    constexpr bool operator<=(Angle rhs) const { return radians_ <= rhs.radians_; }
    constexpr bool operator>(Angle rhs) const { return radians_ > rhs.radians_; }
    constexpr bool operator>=(Angle rhs) const { return radians_ >= rhs.radians_; }

    Angle normalizedSigned() const
    {
        float value = std::remainder(radians_, twoPi());
        return Angle(value);
    }

    Angle normalizedUnsigned() const
    {
        float value = std::fmod(radians_, twoPi());
        if (value < 0.0f) {
            value += twoPi();
        }
        return Angle(value);
    }

    static constexpr float pi() { return 3.14159265358979323846f; }
    static constexpr float twoPi() { return 6.28318530717958647692f; }
    static constexpr Angle zero() { return Angle(0.0f); }
    static constexpr Angle quarterTurn() { return Angle(pi() * 0.5f); }
    static constexpr Angle halfTurn() { return Angle(pi()); }
    static constexpr Angle fullTurn() { return Angle(twoPi()); }

private:
    explicit constexpr Angle(float radiansValue) : radians_(radiansValue) {}
    float radians_ = 0.0f;
};

constexpr Angle operator*(float scalar, Angle value) { return value * scalar; }

inline float sin(Angle value) { return std::sin(value.asRadians()); }
inline float cos(Angle value) { return std::cos(value.asRadians()); }
inline float tan(Angle value) { return std::tan(value.asRadians()); }

} // namespace peco
