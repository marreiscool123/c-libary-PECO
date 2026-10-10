#pragma once

#include <algorithm>
#include <vector>

#include <peco\color.hpp>

namespace peco {

enum class GradientSpread {
    Clamp,
    Repeat,
    Reflect
};

struct GradientStop {
    float position = 0.0f;
    Color color = Color::white();
};

class Gradient {
public:
    Gradient() = default;
    Gradient(Color start, Color end)
    {
        addStop(0.0f, start);
        addStop(1.0f, end);
    }

    void addStop(float position, Color color)
    {
        const float clamped = std::clamp(position, 0.0f, 1.0f);
        const auto iterator = std::lower_bound(stops_.begin(), stops_.end(), clamped,
            [](const GradientStop& stop, float value) {
                return stop.position < value;
            });
        if (iterator != stops_.end() && iterator->position == clamped) {
            iterator->color = color;
        } else {
            stops_.insert(iterator, {clamped, color});
        }
    }
    bool removeStop(float position)
    {
        const auto iterator = std::find_if(stops_.begin(), stops_.end(),
            [position](const GradientStop& stop) { return stop.position == position; });
        if (iterator == stops_.end()) {
            return false;
        }
        stops_.erase(iterator);
        return true;
    }
    void clear() { stops_.clear(); }
    bool empty() const { return stops_.empty(); }
    std::size_t size() const { return stops_.size(); }
    const std::vector<GradientStop>& stops() const { return stops_; }
    void setSpread(GradientSpread spread) { spread_ = spread; }
    GradientSpread spread() const { return spread_; }

    Color sample(float position) const
    {
        if (stops_.empty()) {
            return Color::transparent();
        }
        if (stops_.size() == 1) {
            return stops_.front().color;
        }
        const float t = applySpread(position);
        const auto upper = std::upper_bound(stops_.begin(), stops_.end(), t,
            [](float value, const GradientStop& stop) {
                return value < stop.position;
            });
        if (upper == stops_.begin()) {
            return upper->color;
        }
        if (upper == stops_.end()) {
            return stops_.back().color;
        }
        const GradientStop& right = *upper;
        const GradientStop& left = *(upper - 1);
        const float interval = right.position - left.position;
        const float local = interval > 0.0f ? (t - left.position) / interval : 0.0f;
        return Color::lerp(left.color, right.color, local);
    }

private:
    float applySpread(float position) const
    {
        switch (spread_) {
        case GradientSpread::Clamp:
            return std::clamp(position, 0.0f, 1.0f);
        case GradientSpread::Repeat: {
            const float fraction = position - std::floor(position);
            return fraction;
        }
        case GradientSpread::Reflect: {
            float fraction = std::fmod(position, 2.0f);
            if (fraction < 0.0f) {
                fraction += 2.0f;
            }
            return fraction <= 1.0f ? fraction : 2.0f - fraction;
        }
        }
        return std::clamp(position, 0.0f, 1.0f);
    }

    std::vector<GradientStop> stops_;
    GradientSpread spread_ = GradientSpread::Clamp;
};

class LinearGradient {
public:
    LinearGradient() = default;
    LinearGradient(float startX, float startY, float endX, float endY,
                   Gradient gradient)
        : startX_(startX), startY_(startY), endX_(endX), endY_(endY),
          gradient_(std::move(gradient)) {}

    void setStart(float x, float y) { startX_ = x; startY_ = y; }
    void setEnd(float x, float y) { endX_ = x; endY_ = y; }
    Gradient& gradient() { return gradient_; }
    const Gradient& gradient() const { return gradient_; }
    Color sample(float x, float y) const
    {
        const float dx = endX_ - startX_;
        const float dy = endY_ - startY_;
        const float lengthSquared = dx * dx + dy * dy;
        if (lengthSquared <= 0.0f) {
            return gradient_.sample(0.0f);
        }
        const float t = ((x - startX_) * dx + (y - startY_) * dy) / lengthSquared;
        return gradient_.sample(t);
    }

private:
    float startX_ = 0.0f;
    float startY_ = 0.0f;
    float endX_ = 1.0f;
    float endY_ = 0.0f;
    Gradient gradient_;
};

} // namespace peco
