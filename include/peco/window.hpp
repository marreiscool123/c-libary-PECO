#pragma once

#include <functional>
#include <string>
#include <utility>

#include <peco\platform.hpp>
#include <peco\color.hpp>
#include <peco\input.hpp>

namespace peco {

class Window {
public:
    using PaintHandler = std::function<void(HDC, int, int)>;
    using ResizeHandler = std::function<void(int, int)>;
    using CloseHandler = std::function<bool()>;
    using CommandHandler = std::function<void(int, int, HWND)>;

    Window(const std::wstring& title, int width, int height);
    Window(const std::wstring& title, int width, int height, DWORD style);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void show(int showCommand = SW_SHOW);
    void hide();
    void close();
    void invalidate();
    bool setTitle(const std::wstring& title);
    bool resizeClient(int width, int height);
    bool center();
    void setMinimumClientSize(int width, int height);
    void setResizeHandler(ResizeHandler handler) { resizeHandler_ = std::move(handler); }
    void setCloseHandler(CloseHandler handler) { closeHandler_ = std::move(handler); }
    void setCommandHandler(CommandHandler handler) { commandHandler_ = std::move(handler); }
    HWND handle() const { return handle_; }
    bool isOpen() const { return handle_ != nullptr && IsWindow(handle_) != FALSE; }
    bool isVisible() const { return handle_ != nullptr && IsWindowVisible(handle_) != FALSE; }
    int width() const { return width_; }
    int height() const { return height_; }
    RECT clientBounds() const;
    Input& input() { return input_; }
    const Input& input() const { return input_; }
    void setPaintHandler(PaintHandler handler) { paintHandler_ = std::move(handler); }

private:
    static LRESULT CALLBACK windowProcedure(HWND handle, UINT message,
                                            WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(HWND handle, UINT message, WPARAM wParam, LPARAM lParam);
    bool constrainMinimumSize(LPARAM lParam) const;

    HWND handle_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    int minimumWidth_ = 0;
    int minimumHeight_ = 0;
    DWORD style_ = WS_OVERLAPPEDWINDOW;
    Input input_;
    PaintHandler paintHandler_;
    ResizeHandler resizeHandler_;
    CloseHandler closeHandler_;
    CommandHandler commandHandler_;
};

} // namespace peco
