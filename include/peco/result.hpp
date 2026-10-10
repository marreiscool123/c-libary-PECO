#pragma once

#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace peco {

template<class T, class E>
class Result {
public:
    static Result success(T value)
    {
        return Result(std::in_place_index<0>, std::move(value));
    }
    static Result failure(E error)
    {
        return Result(std::in_place_index<1>, std::move(error));
    }

    bool hasValue() const { return value_.index() == 0; }
    explicit operator bool() const { return hasValue(); }

    T& value() &
    {
        if (!hasValue()) {
            throw std::logic_error("Accessed the value of a failed Peco result.");
        }
        return std::get<0>(value_);
    }
    const T& value() const&
    {
        if (!hasValue()) {
            throw std::logic_error("Accessed the value of a failed Peco result.");
        }
        return std::get<0>(value_);
    }
    T&& value() &&
    {
        if (!hasValue()) {
            throw std::logic_error("Accessed the value of a failed Peco result.");
        }
        return std::move(std::get<0>(value_));
    }
    E& error() &
    {
        if (hasValue()) {
            throw std::logic_error("Accessed the error of a successful Peco result.");
        }
        return std::get<1>(value_);
    }
    const E& error() const&
    {
        if (hasValue()) {
            throw std::logic_error("Accessed the error of a successful Peco result.");
        }
        return std::get<1>(value_);
    }
    T valueOr(T fallback) const&
    {
        return hasValue() ? std::get<0>(value_) : std::move(fallback);
    }

    template<class Function>
    auto map(Function&& function) const
        -> Result<typename std::decay<decltype(function(std::declval<const T&>()))>::type, E>
    {
        using Output = typename std::decay<
            decltype(function(std::declval<const T&>()))>::type;
        if (hasValue()) {
            return Result<Output, E>::success(function(std::get<0>(value_)));
        }
        return Result<Output, E>::failure(std::get<1>(value_));
    }

    template<class Function>
    auto andThen(Function&& function) const
        -> decltype(function(std::declval<const T&>()))
    {
        using Output = decltype(function(std::declval<const T&>()));
        if (hasValue()) {
            return function(std::get<0>(value_));
        }
        return Output::failure(std::get<1>(value_));
    }

private:
    template<class... Args>
    explicit Result(std::in_place_index_t<0> tag, Args&&... args)
        : value_(tag, std::forward<Args>(args)...) {}
    template<class... Args>
    explicit Result(std::in_place_index_t<1> tag, Args&&... args)
        : value_(tag, std::forward<Args>(args)...) {}

    std::variant<T, E> value_;
};

template<class E>
class Result<void, E> {
public:
    static Result success() { return Result(true, E{}); }
    static Result failure(E error) { return Result(false, std::move(error)); }

    bool hasValue() const { return succeeded_; }
    explicit operator bool() const { return hasValue(); }
    void value() const
    {
        if (!succeeded_) {
            throw std::logic_error("Accessed the value of a failed Peco result.");
        }
    }
    E& error() &
    {
        if (succeeded_) {
            throw std::logic_error("Accessed the error of a successful Peco result.");
        }
        return error_;
    }
    const E& error() const&
    {
        if (succeeded_) {
            throw std::logic_error("Accessed the error of a successful Peco result.");
        }
        return error_;
    }

private:
    Result(bool succeeded, E error)
        : succeeded_(succeeded), error_(std::move(error)) {}
    bool succeeded_;
    E error_;
};

} // namespace peco
