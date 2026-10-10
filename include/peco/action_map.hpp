#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <peco\input.hpp>

namespace peco {

struct KeyBinding {
    int key = 0;
    bool requireControl = false;
    bool requireShift = false;
    bool requireAlt = false;
};

class ActionMap {
public:
    void bindKey(const std::string& action, int virtualKey,
                 bool control = false, bool shift = false, bool alt = false)
    {
        actions_[action].keys.push_back({virtualKey, control, shift, alt});
    }
    void bindMouseButton(const std::string& action, int virtualKey)
    {
        actions_[action].mouseButtons.push_back(virtualKey);
    }
    void bindAxis(const std::string& axis, int negativeKey, int positiveKey)
    {
        axes_[axis] = {negativeKey, positiveKey};
    }
    void clearAction(const std::string& action)
    {
        actions_.erase(action);
    }
    void clearAxis(const std::string& axis)
    {
        axes_.erase(axis);
    }
    void clear()
    {
        actions_.clear();
        axes_.clear();
    }
    bool hasAction(const std::string& action) const
    {
        return actions_.find(action) != actions_.end();
    }
    bool hasAxis(const std::string& axis) const
    {
        return axes_.find(axis) != axes_.end();
    }
    bool down(const std::string& action, const Input& input) const
    {
        const auto found = actions_.find(action);
        if (found == actions_.end()) {
            return false;
        }
        for (const KeyBinding& binding : found->second.keys) {
            if (input.keyDown(binding.key) && modifierMatches(binding, input)) {
                return true;
            }
        }
        for (const int button : found->second.mouseButtons) {
            if (input.mouseButtonDown(button)) {
                return true;
            }
        }
        return false;
    }
    bool pressed(const std::string& action, const Input& input) const
    {
        const auto found = actions_.find(action);
        if (found == actions_.end()) {
            return false;
        }
        for (const KeyBinding& binding : found->second.keys) {
            if (input.keyPressed(binding.key) && modifierMatches(binding, input)) {
                return true;
            }
        }
        for (const int button : found->second.mouseButtons) {
            if (input.mouseButtonPressed(button)) {
                return true;
            }
        }
        return false;
    }
    bool released(const std::string& action, const Input& input) const
    {
        const auto found = actions_.find(action);
        if (found == actions_.end()) {
            return false;
        }
        for (const KeyBinding& binding : found->second.keys) {
            if (input.keyReleased(binding.key)) {
                return true;
            }
        }
        for (const int button : found->second.mouseButtons) {
            if (input.mouseButtonReleased(button)) {
                return true;
            }
        }
        return false;
    }
    float axis(const std::string& name, const Input& input) const
    {
        const auto found = axes_.find(name);
        if (found == axes_.end()) {
            return 0.0f;
        }
        const bool negative = input.keyDown(found->second.first);
        const bool positive = input.keyDown(found->second.second);
        return static_cast<float>(positive) - static_cast<float>(negative);
    }
    Vec2 mouseDelta(const Input& input, float scale = 1.0f) const
    {
        const POINT delta = input.mouseDelta();
        return {static_cast<float>(delta.x) * scale,
                static_cast<float>(delta.y) * scale};
    }
    float scroll(const Input& input, float wheelNotches = 120.0f) const
    {
        return wheelNotches == 0.0f ? 0.0f
            : static_cast<float>(input.wheelDelta()) / wheelNotches;
    }
    std::size_t actionCount() const { return actions_.size(); }
    std::size_t axisCount() const { return axes_.size(); }

private:
    struct Action {
        std::vector<KeyBinding> keys;
        std::vector<int> mouseButtons;
    };

    static bool modifierMatches(const KeyBinding& binding, const Input& input)
    {
        return (!binding.requireControl || input.keyDown(VK_CONTROL))
            && (!binding.requireShift || input.keyDown(VK_SHIFT))
            && (!binding.requireAlt || input.keyDown(VK_MENU));
    }

    std::unordered_map<std::string, Action> actions_;
    std::unordered_map<std::string, std::pair<int, int>> axes_;
};

} // namespace peco
