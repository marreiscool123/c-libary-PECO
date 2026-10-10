#pragma once

#include <chrono>

namespace peco {

class Timer {
public:
    Timer();
    float tick();
    void reset();

private:
    using Clock = std::chrono::steady_clock;
    Clock::time_point previous_;
};

} // namespace peco
