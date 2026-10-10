#pragma once

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include <peco\platform.hpp>

namespace peco::ui {

class NativeControl {
public:
    NativeControl() = default;
    NativeControl(const NativeControl&) = delete;
    NativeControl& operator=(const NativeControl&) = delete;
    NativeControl(NativeControl&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr)),
          parent_(std::exchange(other.parent_, nullptr)),
          id_(std::exchange(other.id_, 0)),
          lastError_(std::exchange(other.lastError_, ERROR_SUCCESS)) {}
    NativeControl& operator=(NativeControl&& other) noexcept
    {
        if (this != &other) {
            destroy();
            handle_ = std::exchange(other.handle_, nullptr);
            parent_ = std::exchange(other.parent_, nullptr);
            id_ = std::exchange(other.id_, 0);
            lastError_ = std::exchange(other.lastError_, ERROR_SUCCESS);
        }
        return *this;
    }
    virtual ~NativeControl() { destroy(); }

    bool create(HWND parent, const wchar_t* className, const std::wstring& text,
                int id, const RECT& bounds, DWORD style,
                DWORD extendedStyle = 0)
    {
        destroy();
        if (parent == nullptr || className == nullptr) {
            lastError_ = ERROR_INVALID_PARAMETER;
            return false;
        }
        handle_ = CreateWindowExW(
            extendedStyle, className, text.c_str(), style | WS_CHILD,
            bounds.left, bounds.top, bounds.right - bounds.left,
            bounds.bottom - bounds.top, parent,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
            GetModuleHandleW(nullptr), nullptr);
        if (handle_ == nullptr) {
            lastError_ = GetLastError();
            parent_ = nullptr;
            id_ = 0;
            return false;
        }
        parent_ = parent;
        id_ = id;
        lastError_ = ERROR_SUCCESS;
        return true;
    }
    void destroy()
    {
        if (handle_ != nullptr && IsWindow(handle_)) {
            DestroyWindow(handle_);
        }
        handle_ = nullptr;
        parent_ = nullptr;
        id_ = 0;
    }
    bool isCreated() const { return handle_ != nullptr && IsWindow(handle_) != FALSE; }
    HWND handle() const { return handle_; }
    HWND parent() const { return parent_; }
    int id() const { return id_; }
    DWORD lastError() const { return lastError_; }

    bool setText(const std::wstring& value)
    {
        if (!isCreated()) {
            return false;
        }
        const BOOL success = SetWindowTextW(handle_, value.c_str());
        lastError_ = success ? ERROR_SUCCESS : GetLastError();
        return success != FALSE;
    }
    std::wstring text() const
    {
        if (!isCreated()) {
            return {};
        }
        const int length = GetWindowTextLengthW(handle_);
        if (length <= 0) {
            return {};
        }
        std::wstring value(static_cast<std::size_t>(length) + 1, L'\0');
        const int copied = GetWindowTextW(handle_, &value[0], length + 1);
        value.resize(static_cast<std::size_t>(copied));
        return value;
    }
    bool setBounds(const RECT& bounds)
    {
        if (!isCreated()) {
            return false;
        }
        const BOOL success = MoveWindow(handle_, bounds.left, bounds.top,
            bounds.right - bounds.left, bounds.bottom - bounds.top, TRUE);
        lastError_ = success ? ERROR_SUCCESS : GetLastError();
        return success != FALSE;
    }
    bool bounds(RECT& result) const
    {
        if (!isCreated() || !GetWindowRect(handle_, &result)) {
            return false;
        }
        MapWindowPoints(nullptr, parent_, reinterpret_cast<POINT*>(&result), 2);
        return true;
    }
    bool show(int command = SW_SHOW)
    {
        if (!isCreated()) {
            return false;
        }
        ShowWindow(handle_, command);
        return true;
    }
    bool hide() { return show(SW_HIDE); }
    bool setEnabled(bool enabled)
    {
        if (!isCreated()) {
            return false;
        }
        EnableWindow(handle_, enabled ? TRUE : FALSE);
        return true;
    }
    bool enabled() const
    {
        return isCreated() && IsWindowEnabled(handle_) != FALSE;
    }
    bool visible() const
    {
        return isCreated() && IsWindowVisible(handle_) != FALSE;
    }
    bool focus()
    {
        if (!isCreated()) {
            return false;
        }
        SetFocus(handle_);
        return GetFocus() == handle_;
    }
    bool invalidate()
    {
        return isCreated() && InvalidateRect(handle_, nullptr, TRUE) != FALSE;
    }

protected:
    HWND handle_ = nullptr;
    HWND parent_ = nullptr;
    int id_ = 0;
    mutable DWORD lastError_ = ERROR_SUCCESS;
};

class Button : public NativeControl {
public:
    bool create(HWND parent, int id, const std::wstring& label, const RECT& bounds,
                bool defaultButton = false)
    {
        const DWORD buttonStyle = WS_TABSTOP | WS_VISIBLE
            | (defaultButton ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON);
        return NativeControl::create(parent, L"BUTTON", label, id, bounds, buttonStyle);
    }
    bool click()
    {
        if (!isCreated()) {
            return false;
        }
        SendMessageW(handle_, BM_CLICK, 0, 0);
        return true;
    }
};

class Label : public NativeControl {
public:
    bool create(HWND parent, int id, const std::wstring& label, const RECT& bounds,
                DWORD alignment = SS_LEFT)
    {
        return NativeControl::create(parent, L"STATIC", label, id, bounds,
            WS_VISIBLE | SS_NOTIFY | alignment);
    }
};

class CheckBox : public NativeControl {
public:
    bool create(HWND parent, int id, const std::wstring& label,
                const RECT& bounds, bool checked = false)
    {
        const bool created = NativeControl::create(parent, L"BUTTON", label, id,
            bounds, WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX);
        if (created) {
            setChecked(checked);
        }
        return created;
    }
    bool checked() const
    {
        return isCreated() && SendMessageW(handle_, BM_GETCHECK, 0, 0) == BST_CHECKED;
    }
    bool setChecked(bool value)
    {
        if (!isCreated()) {
            return false;
        }
        SendMessageW(handle_, BM_SETCHECK, value ? BST_CHECKED : BST_UNCHECKED, 0);
        return true;
    }
};

class EditControl : public NativeControl {
public:
    bool create(HWND parent, int id, const std::wstring& value,
                const RECT& bounds, bool multiline = false)
    {
        DWORD style = WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL;
        if (multiline) {
            style = WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_MULTILINE
                | ES_AUTOVSCROLL | WS_VSCROLL;
        }
        return NativeControl::create(parent, L"EDIT", value, id, bounds, style);
    }
    bool setReadOnly(bool value)
    {
        return isCreated()
            && SendMessageW(handle_, EM_SETREADONLY, value ? TRUE : FALSE, 0) != 0;
    }
    bool setSelection(int start, int end)
    {
        if (!isCreated()) {
            return false;
        }
        SendMessageW(handle_, EM_SETSEL, static_cast<WPARAM>(start),
                     static_cast<LPARAM>(end));
        return true;
    }
    bool selectAll() { return setSelection(0, -1); }
    bool appendText(const std::wstring& value)
    {
        if (!isCreated()) {
            return false;
        }
        const int length = GetWindowTextLengthW(handle_);
        SendMessageW(handle_, EM_SETSEL, static_cast<WPARAM>(length),
                     static_cast<LPARAM>(length));
        SendMessageW(handle_, EM_REPLACESEL, FALSE,
                     reinterpret_cast<LPARAM>(value.c_str()));
        return true;
    }
    int lineCount() const
    {
        return isCreated() ? static_cast<int>(SendMessageW(handle_, EM_GETLINECOUNT, 0, 0)) : 0;
    }
};

class ComboBox : public NativeControl {
public:
    bool create(HWND parent, int id, const RECT& bounds,
                bool editable = false)
    {
        const DWORD type = editable ? CBS_DROPDOWN : CBS_DROPDOWNLIST;
        const DWORD style = WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | type;
        return NativeControl::create(parent, L"COMBOBOX", L"", id, bounds, style);
    }
    int addItem(const std::wstring& item)
    {
        return isCreated() ? static_cast<int>(SendMessageW(handle_, CB_ADDSTRING, 0,
            reinterpret_cast<LPARAM>(item.c_str()))) : CB_ERR;
    }
    bool insertItem(int index, const std::wstring& item)
    {
        if (!isCreated()) {
            return false;
        }
        return SendMessageW(handle_, CB_INSERTSTRING, static_cast<WPARAM>(index),
            reinterpret_cast<LPARAM>(item.c_str())) != CB_ERR;
    }
    bool removeItem(int index)
    {
        return isCreated()
            && SendMessageW(handle_, CB_DELETESTRING, static_cast<WPARAM>(index), 0) != CB_ERR;
    }
    void clearItems()
    {
        if (isCreated()) {
            SendMessageW(handle_, CB_RESETCONTENT, 0, 0);
        }
    }
    int itemCount() const
    {
        return isCreated() ? static_cast<int>(SendMessageW(handle_, CB_GETCOUNT, 0, 0)) : 0;
    }
    int selectedIndex() const
    {
        return isCreated() ? static_cast<int>(SendMessageW(handle_, CB_GETCURSEL, 0, 0)) : CB_ERR;
    }
    bool setSelectedIndex(int index)
    {
        if (!isCreated() || index < -1 || index >= itemCount()) {
            return false;
        }
        SendMessageW(handle_, CB_SETCURSEL, static_cast<WPARAM>(index), 0);
        return true;
    }
    std::wstring itemText(int index) const
    {
        if (!isCreated()) {
            return {};
        }
        const LRESULT length = SendMessageW(handle_, CB_GETLBTEXTLEN,
                                             static_cast<WPARAM>(index), 0);
        if (length == CB_ERR || length < 0) {
            return {};
        }
        std::wstring value(static_cast<std::size_t>(length) + 1, L'\0');
        const LRESULT copied = SendMessageW(handle_, CB_GETLBTEXT,
            static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(&value[0]));
        if (copied == CB_ERR) {
            return {};
        }
        value.resize(static_cast<std::size_t>(copied));
        return value;
    }
};

class RadioButton : public NativeControl {
public:
    bool create(HWND parent, int id, const std::wstring& label,
                const RECT& bounds, bool selected = false,
                bool beginGroup = false)
    {
        const DWORD style = WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON
            | (beginGroup ? WS_GROUP : 0);
        const bool created = NativeControl::create(
            parent, L"BUTTON", label, id, bounds, style);
        if (created && selected) {
            setSelected(true);
        }
        return created;
    }
    bool selected() const
    {
        return isCreated()
            && SendMessageW(handle_, BM_GETCHECK, 0, 0) == BST_CHECKED;
    }
    bool setSelected(bool value)
    {
        if (!isCreated()) {
            return false;
        }
        SendMessageW(handle_, BM_SETCHECK,
                     value ? BST_CHECKED : BST_UNCHECKED, 0);
        return true;
    }
};

class GroupBox : public NativeControl {
public:
    bool create(HWND parent, int id, const std::wstring& label,
                const RECT& bounds)
    {
        return NativeControl::create(parent, L"BUTTON", label, id, bounds,
                                     WS_VISIBLE | BS_GROUPBOX);
    }
};

class ListBox : public NativeControl {
public:
    bool create(HWND parent, int id, const RECT& bounds,
                bool multipleSelection = false, bool sorted = false)
    {
        DWORD style = WS_VISIBLE | WS_TABSTOP | WS_BORDER
            | WS_VSCROLL | LBS_NOTIFY;
        if (multipleSelection) {
            style |= LBS_EXTENDEDSEL;
        }
        if (sorted) {
            style |= LBS_SORT;
        }
        return NativeControl::create(parent, L"LISTBOX", L"", id, bounds, style);
    }
    int addItem(const std::wstring& item)
    {
        return isCreated()
            ? static_cast<int>(SendMessageW(handle_, LB_ADDSTRING, 0,
                                             reinterpret_cast<LPARAM>(item.c_str())))
            : LB_ERR;
    }
    bool insertItem(int index, const std::wstring& item)
    {
        return isCreated()
            && SendMessageW(handle_, LB_INSERTSTRING, static_cast<WPARAM>(index),
                reinterpret_cast<LPARAM>(item.c_str())) != LB_ERR;
    }
    bool removeItem(int index)
    {
        return isCreated()
            && SendMessageW(handle_, LB_DELETESTRING, static_cast<WPARAM>(index), 0) != LB_ERR;
    }
    void clearItems()
    {
        if (isCreated()) {
            SendMessageW(handle_, LB_RESETCONTENT, 0, 0);
        }
    }
    int itemCount() const
    {
        return isCreated()
            ? static_cast<int>(SendMessageW(handle_, LB_GETCOUNT, 0, 0)) : 0;
    }
    int selectedIndex() const
    {
        return isCreated()
            ? static_cast<int>(SendMessageW(handle_, LB_GETCURSEL, 0, 0)) : LB_ERR;
    }
    bool setSelectedIndex(int index)
    {
        if (!isCreated() || index < -1 || index >= itemCount()) {
            return false;
        }
        SendMessageW(handle_, LB_SETCURSEL, static_cast<WPARAM>(index), 0);
        return true;
    }
    std::wstring itemText(int index) const
    {
        if (!isCreated()) {
            return {};
        }
        const LRESULT length = SendMessageW(handle_, LB_GETTEXTLEN,
                                             static_cast<WPARAM>(index), 0);
        if (length == LB_ERR || length < 0) {
            return {};
        }
        std::wstring value(static_cast<std::size_t>(length) + 1, L'\0');
        const LRESULT copied = SendMessageW(handle_, LB_GETTEXT,
            static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(&value[0]));
        if (copied == LB_ERR) {
            return {};
        }
        value.resize(static_cast<std::size_t>(copied));
        return value;
    }
};

} // namespace peco::ui
