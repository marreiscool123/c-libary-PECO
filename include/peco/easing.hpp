#pragma once

#include <cmath>

#include <peco\functions.hpp>

namespace peco::easing {

inline float linear(float t) { return math::saturate(t); }
inline float inSine(float t) { return 1.0f - std::cos(math::saturate(t) * math::pi * 0.5f); }
inline float outSine(float t) { return std::sin(math::saturate(t) * math::pi * 0.5f); }
inline float inOutSine(float t) { return -(std::cos(math::pi * math::saturate(t)) - 1.0f) * 0.5f; }
inline float inQuad(float t) { t = math::saturate(t); return t * t; }
inline float outQuad(float t) { t = math::saturate(t); return 1.0f - (1.0f - t) * (1.0f - t); }
inline float inOutQuad(float t)
{
    t = math::saturate(t);
    return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) * 0.5f;
}
inline float inCubic(float t) { t = math::saturate(t); return t * t * t; }
inline float outCubic(float t) { t = math::saturate(t); return 1.0f - std::pow(1.0f - t, 3.0f); }
inline float inOutCubic(float t)
{
    t = math::saturate(t);
    return t < 0.5f ? 4.0f * t * t * t
        : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f;
}
inline float inQuart(float t) { t = math::saturate(t); return t * t * t * t; }
inline float outQuart(float t) { t = math::saturate(t); return 1.0f - std::pow(1.0f - t, 4.0f); }
inline float inOutQuart(float t)
{
    t = math::saturate(t);
    return t < 0.5f ? 8.0f * t * t * t * t
        : 1.0f - std::pow(-2.0f * t + 2.0f, 4.0f) * 0.5f;
}
inline float inQuint(float t) { t = math::saturate(t); return t * t * t * t * t; }
inline float outQuint(float t) { t = math::saturate(t); return 1.0f - std::pow(1.0f - t, 5.0f); }
inline float inOutQuint(float t)
{
    t = math::saturate(t);
    return t < 0.5f ? 16.0f * t * t * t * t * t
        : 1.0f - std::pow(-2.0f * t + 2.0f, 5.0f) * 0.5f;
}
inline float inExpo(float t)
{
    t = math::saturate(t);
    return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * t - 10.0f);
}
inline float outExpo(float t)
{
    t = math::saturate(t);
    return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
}
inline float inOutExpo(float t)
{
    t = math::saturate(t);
    if (t == 0.0f || t == 1.0f) return t;
    return t < 0.5f ? std::pow(2.0f, 20.0f * t - 10.0f) * 0.5f
        : (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) * 0.5f;
}
inline float inCirc(float t)
{
    t = math::saturate(t);
    return 1.0f - std::sqrt(1.0f - t * t);
}
inline float outCirc(float t)
{
    t = math::saturate(t);
    return std::sqrt(1.0f - (t - 1.0f) * (t - 1.0f));
}
inline float inOutCirc(float t)
{
    t = math::saturate(t);
    return t < 0.5f
        ? (1.0f - std::sqrt(1.0f - 4.0f * t * t)) * 0.5f
        : (std::sqrt(1.0f - std::pow(-2.0f * t + 2.0f, 2.0f)) + 1.0f) * 0.5f;
}
inline float inBack(float t, float overshoot = 1.70158f)
{
    t = math::saturate(t);
    return (overshoot + 1.0f) * t * t * t - overshoot * t * t;
}
inline float outBack(float t, float overshoot = 1.70158f)
{
    t = math::saturate(t) - 1.0f;
    return 1.0f + (overshoot + 1.0f) * t * t * t + overshoot * t * t;
}
inline float inOutBack(float t, float overshoot = 1.70158f)
{
    t = math::saturate(t) * 2.0f;
    const float scaledOvershoot = overshoot * 1.525f;
    return t < 1.0f
        ? t * t * ((scaledOvershoot + 1.0f) * t - scaledOvershoot) * 0.5f
        : (t - 2.0f) * (t - 2.0f)
            * ((scaledOvershoot + 1.0f) * (t - 2.0f) + scaledOvershoot) * 0.5f + 1.0f;
}
inline float outBounce(float t)
{
    t = math::saturate(t);
    constexpr float n = 7.5625f;
    constexpr float d = 2.75f;
    if (t < 1.0f / d) return n * t * t;
    if (t < 2.0f / d) {
        t -= 1.5f / d;
        return n * t * t + 0.75f;
    }
    if (t < 2.5f / d) {
        t -= 2.25f / d;
        return n * t * t + 0.9375f;
    }
    t -= 2.625f / d;
    return n * t * t + 0.984375f;
}
inline float inBounce(float t) { return 1.0f - outBounce(1.0f - math::saturate(t)); }
inline float inOutBounce(float t)
{
    t = math::saturate(t);
    return t < 0.5f ? (1.0f - outBounce(1.0f - 2.0f * t)) * 0.5f
        : (1.0f + outBounce(2.0f * t - 1.0f)) * 0.5f;
}
inline float outElastic(float t, float period = 0.3f)
{
    t = math::saturate(t);
    if (t == 0.0f || t == 1.0f) return t;
    const float phase = period / 4.0f;
    return std::pow(2.0f, -10.0f * t)
        * std::sin((t - phase) * math::tau / period) + 1.0f;
}
inline float inElastic(float t, float period = 0.3f)
{
    return 1.0f - outElastic(1.0f - math::saturate(t), period);
}
inline float inOutElastic(float t, float period = 0.45f)
{
    t = math::saturate(t);
    if (t == 0.0f || t == 1.0f) return t;
    const float phase = period / 4.0f;
    t = 2.0f * t - 1.0f;
    if (t < 0.0f) {
        return -0.5f * std::pow(2.0f, 10.0f * t)
            * std::sin((t - phase) * math::tau / period);
    }
    return std::pow(2.0f, -10.0f * t)
        * std::sin((t - phase) * math::tau / period) * 0.5f + 1.0f;
}

} // namespace peco::easing
