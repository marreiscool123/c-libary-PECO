#pragma once

#include <type_traits>
#include <utility>

namespace peco {

template<class Function>
class ScopeGuard {
public:
    explicit ScopeGuard(Function function)
        : function_(std::move(function)) {}
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&& other) noexcept(
        std::is_nothrow_move_constructible<Function>::value)
        : function_(std::move(other.function_)), active_(other.active_)
    {
        other.active_ = false;
    }
    ScopeGuard& operator=(ScopeGuard&&) = delete;
    ~ScopeGuard() noexcept
    {
        if (active_) {
            function_();
        }
    }

    void dismiss() noexcept { active_ = false; }
    bool active() const noexcept { return active_; }
    void run()
    {
        if (active_) {
            active_ = false;
            function_();
        }
    }

private:
    Function function_;
    bool active_ = true;
};

template<class Function>
ScopeGuard<typename std::decay<Function>::type> makeScopeGuard(Function&& function)
{
    return ScopeGuard<typename std::decay<Function>::type>(
        std::forward<Function>(function));
}

} // namespace peco
