#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#include <peco\rect.hpp>

namespace peco::ui {

enum class Direction {
    Horizontal,
    Vertical
};

enum class MainAlignment {
    Start,
    Center,
    End,
    SpaceBetween,
    SpaceAround
};

enum class CrossAlignment {
    Start,
    Center,
    End,
    Stretch
};

struct Insets {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;

    constexpr Insets() = default;
    constexpr explicit Insets(float uniform)
        : left(uniform), top(uniform), right(uniform), bottom(uniform) {}
    constexpr Insets(float horizontal, float vertical)
        : left(horizontal), top(vertical), right(horizontal), bottom(vertical) {}
    constexpr Insets(float leftValue, float topValue,
                     float rightValue, float bottomValue)
        : left(leftValue), top(topValue), right(rightValue), bottom(bottomValue) {}
    constexpr float horizontal() const { return left + right; }
    constexpr float vertical() const { return top + bottom; }
};

struct LayoutItem {
    Size2 preferred{};
    Size2 minimum{};
    Size2 maximum{1.0e9f, 1.0e9f};
    Insets margin{};
    float grow = 0.0f;
    float shrink = 1.0f;
    bool visible = true;
};

class FlexLayout {
public:
    explicit FlexLayout(Direction direction = Direction::Horizontal)
        : direction_(direction) {}

    void setDirection(Direction value) { direction_ = value; }
    void setGap(float value) { gap_ = std::max(0.0f, value); }
    void setPadding(Insets value) { padding_ = value; }
    void setMainAlignment(MainAlignment value) { mainAlignment_ = value; }
    void setCrossAlignment(CrossAlignment value) { crossAlignment_ = value; }
    Direction direction() const { return direction_; }
    float gap() const { return gap_; }
    Insets padding() const { return padding_; }
    MainAlignment mainAlignment() const { return mainAlignment_; }
    CrossAlignment crossAlignment() const { return crossAlignment_; }

    std::vector<Rect> arrange(const Rect& bounds,
                              const std::vector<LayoutItem>& items) const
    {
        std::vector<Rect> result(items.size());
        const float availableMain = std::max(0.0f,
            mainExtent(bounds) - mainBefore(padding_) - mainAfter(padding_));
        const float availableCross = std::max(0.0f,
            crossExtent(bounds) - crossBefore(padding_) - crossAfter(padding_));
        std::vector<std::size_t> visible;
        visible.reserve(items.size());
        for (std::size_t index = 0; index < items.size(); ++index) {
            if (items[index].visible) {
                visible.push_back(index);
            }
        }
        if (visible.empty()) {
            return result;
        }

        const std::size_t count = visible.size();
        float totalGap = gap_ * static_cast<float>(count - 1);
        float totalPreferred = 0.0f;
        float totalGrow = 0.0f;
        for (const std::size_t index : visible) {
            totalPreferred += mainPreferred(items[index])
                + mainBefore(items[index].margin) + mainAfter(items[index].margin);
            totalGrow += std::max(0.0f, items[index].grow);
        }
        float freeSpace = availableMain - totalGap - totalPreferred;
        std::vector<float> extents(count);
        for (std::size_t slot = 0; slot < count; ++slot) {
            const LayoutItem& item = items[visible[slot]];
            extents[slot] = clampMain(item, mainPreferred(item));
        }

        if (freeSpace > 0.0f && totalGrow > 0.0f) {
            for (std::size_t slot = 0; slot < count; ++slot) {
                const LayoutItem& item = items[visible[slot]];
                const float share = freeSpace * std::max(0.0f, item.grow) / totalGrow;
                extents[slot] = clampMain(item, extents[slot] + share);
            }
        } else if (freeSpace < 0.0f) {
            const float deficit = -freeSpace;
            float totalShrink = 0.0f;
            for (std::size_t slot = 0; slot < count; ++slot) {
                totalShrink += extents[slot] * std::max(0.0f, items[visible[slot]].shrink);
            }
            if (totalShrink > 0.0f) {
                for (std::size_t slot = 0; slot < count; ++slot) {
                    const LayoutItem& item = items[visible[slot]];
                    const float weight = extents[slot] * std::max(0.0f, item.shrink);
                    extents[slot] = clampMain(item,
                        extents[slot] - deficit * weight / totalShrink);
                }
            }
        }

        float used = totalGap;
        for (std::size_t slot = 0; slot < count; ++slot) {
            used += extents[slot] + mainBefore(items[visible[slot]].margin)
                + mainAfter(items[visible[slot]].margin);
        }
        float leadingSpace = 0.0f;
        float betweenSpace = gap_;
        const float remaining = std::max(0.0f, availableMain - used);
        switch (mainAlignment_) {
        case MainAlignment::Start:
            break;
        case MainAlignment::Center:
            leadingSpace = remaining * 0.5f;
            break;
        case MainAlignment::End:
            leadingSpace = remaining;
            break;
        case MainAlignment::SpaceBetween:
            if (count > 1) {
                betweenSpace += remaining / static_cast<float>(count - 1);
            }
            break;
        case MainAlignment::SpaceAround:
            betweenSpace += remaining / static_cast<float>(count);
            leadingSpace = (betweenSpace - gap_) * 0.5f;
            break;
        }

        float cursor = mainStart(bounds) + mainBefore(padding_) + leadingSpace;
        for (std::size_t slot = 0; slot < count; ++slot) {
            const std::size_t index = visible[slot];
            const LayoutItem& item = items[index];
            cursor += mainBefore(item.margin);
            const float mainSize = extents[slot];
            const float crossSize = crossSizeFor(item, availableCross);
            const float crossMarginBefore = crossBefore(item.margin);
            const float crossMarginAfter = crossAfter(item.margin);
            const float innerCross = std::max(0.0f,
                availableCross - crossMarginBefore - crossMarginAfter);
            float crossPosition = crossStart(bounds) + crossBefore(padding_)
                + crossMarginBefore;
            if (crossAlignment_ == CrossAlignment::Center) {
                crossPosition += (innerCross - crossSize) * 0.5f;
            } else if (crossAlignment_ == CrossAlignment::End) {
                crossPosition += innerCross - crossSize;
            }
            result[index] = makeRect(cursor, crossPosition, mainSize, crossSize);
            cursor += mainSize + mainAfter(item.margin);
            if (slot + 1 < count) {
                cursor += betweenSpace;
            }
        }
        return result;
    }

private:
    float mainExtent(const Rect& bounds) const
    {
        return direction_ == Direction::Horizontal ? bounds.width : bounds.height;
    }
    float crossExtent(const Rect& bounds) const
    {
        return direction_ == Direction::Horizontal ? bounds.height : bounds.width;
    }
    float mainStart(const Rect& bounds) const
    {
        return direction_ == Direction::Horizontal ? bounds.x : bounds.y;
    }
    float crossStart(const Rect& bounds) const
    {
        return direction_ == Direction::Horizontal ? bounds.y : bounds.x;
    }
    float mainBefore(const Insets& insets) const
    {
        return direction_ == Direction::Horizontal ? insets.left : insets.top;
    }
    float mainAfter(const Insets& insets) const
    {
        return direction_ == Direction::Horizontal ? insets.right : insets.bottom;
    }
    float crossBefore(const Insets& insets) const
    {
        return direction_ == Direction::Horizontal ? insets.top : insets.left;
    }
    float crossAfter(const Insets& insets) const
    {
        return direction_ == Direction::Horizontal ? insets.bottom : insets.right;
    }
    float mainPreferred(const LayoutItem& item) const
    {
        return direction_ == Direction::Horizontal ? item.preferred.width : item.preferred.height;
    }
    float mainMinimum(const LayoutItem& item) const
    {
        return direction_ == Direction::Horizontal ? item.minimum.width : item.minimum.height;
    }
    float mainMaximum(const LayoutItem& item) const
    {
        return direction_ == Direction::Horizontal ? item.maximum.width : item.maximum.height;
    }
    float crossPreferred(const LayoutItem& item) const
    {
        return direction_ == Direction::Horizontal ? item.preferred.height : item.preferred.width;
    }
    float crossMaximum(const LayoutItem& item) const
    {
        return direction_ == Direction::Horizontal ? item.maximum.height : item.maximum.width;
    }
    float clampMain(const LayoutItem& item, float value) const
    {
        return std::clamp(value, mainMinimum(item), mainMaximum(item));
    }
    float crossSizeFor(const LayoutItem& item, float available) const
    {
        const float size = crossAlignment_ == CrossAlignment::Stretch
            ? available - crossBefore(item.margin) - crossAfter(item.margin)
            : crossPreferred(item);
        return std::clamp(size, 0.0f, crossMaximum(item));
    }
    Rect makeRect(float main, float cross, float mainSize, float crossSize) const
    {
        return direction_ == Direction::Horizontal
            ? Rect{main, cross, mainSize, crossSize}
            : Rect{cross, main, crossSize, mainSize};
    }

    Direction direction_;
    MainAlignment mainAlignment_ = MainAlignment::Start;
    CrossAlignment crossAlignment_ = CrossAlignment::Stretch;
    Insets padding_{};
    float gap_ = 0.0f;
};

} // namespace peco::ui
