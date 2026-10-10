#pragma once

#include <functional>

#include <peco\timer.hpp>
#include <peco\window.hpp>

namespace peco {

class Application {
public:
    using UpdateHandler = std::function<void(float)>;
    using RenderHandler = Window::PaintHandler;

    Application(const std::wstring& title, int width, int height);

    Window& window() { return window_; }
    const Window& window() const { return window_; }
    void setUpdateHandler(UpdateHandler handler) { updateHandler_ = std::move(handler); }
    void setRenderHandler(RenderHandler handler)
    {
        window_.setPaintHandler(std::move(handler));
    }
    int run(int showCommand = SW_SHOW);

private:
    Window window_;
    Timer timer_;
    UpdateHandler updateHandler_;
};

} // namespace peco
