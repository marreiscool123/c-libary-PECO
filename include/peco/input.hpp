#pragma once

#include <array>

#include <peco\platform.hpp>

namespace peco {

class Input {
public:
    void beginFrame();
    bool keyDown(int virtualKey) const;
    bool keyPressed(int virtualKey) const;
    bool keyReleased(int virtualKey) const;
    bool mouseButtonDown(int virtualKey) const;
    bool mouseButtonPressed(int virtualKey) const;
    bool mouseButtonReleased(int virtualKey) const;
    POINT mousePosition() const { return mousePosition_; }
    POINT mouseDelta() const { return mouseDelta_; }
    int wheelDelta() const { return wheelDelta_; }
    bool anyKeyDown() const;
    bool anyMouseButtonDown() const;
    bool focused() const { return focused_; }
    void updateMousePosition(int x, int y);

    void processMessage(UINT message, WPARAM wParam, LPARAM lParam);

private:
    std::array<bool, 256> keys_{};
    std::array<bool, 256> keyPressed_{};
    std::array<bool, 256> keyReleased_{};
    std::array<bool, 3> mouseButtons_{};
    std::array<bool, 3> mousePressed_{};
    std::array<bool, 3> mouseReleased_{};
    POINT mousePosition_{};
    POINT mouseDelta_{};
    int wheelDelta_ = 0;
    bool focused_ = true;
};

} // namespace peco
