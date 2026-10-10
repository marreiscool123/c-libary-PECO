#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <exception>
#include <utility>
#include <vector>

namespace peco {

template<class... Args>
class Event {
private:
    struct Slot {
        std::size_t id = 0;
        bool connected = true;
        std::function<void(Args...)> callback;
    };

    struct State {
        std::size_t nextId = 1;
        bool dispatching = false;
        std::vector<std::shared_ptr<Slot>> slots;

        void compact()
        {
            if (dispatching) {
                return;
            }
            slots.erase(std::remove_if(slots.begin(), slots.end(),
                [](const std::shared_ptr<Slot>& slot) { return !slot->connected; }),
                slots.end());
        }
    };

public:
    class Connection {
    public:
        Connection() = default;
        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;
        Connection(Connection&& other) noexcept
            : state_(std::move(other.state_)), slot_(std::move(other.slot_)) {}
        Connection& operator=(Connection&& other) noexcept
        {
            if (this != &other) {
                disconnect();
                state_ = std::move(other.state_);
                slot_ = std::move(other.slot_);
            }
            return *this;
        }
        ~Connection() { disconnect(); }

        void disconnect()
        {
            const auto slot = slot_.lock();
            if (slot) {
                slot->connected = false;
            }
            const auto state = state_.lock();
            if (state) {
                state->compact();
            }
            slot_.reset();
            state_.reset();
        }
        bool connected() const
        {
            const auto slot = slot_.lock();
            return slot && slot->connected;
        }
        explicit operator bool() const { return connected(); }

    private:
        friend class Event;
        Connection(const std::shared_ptr<State>& state,
                   const std::shared_ptr<Slot>& slot)
            : state_(state), slot_(slot) {}
        std::weak_ptr<State> state_;
        std::weak_ptr<Slot> slot_;
    };

    Event() : state_(std::make_shared<State>()) {}
    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;
    Event(Event&&) noexcept = default;
    Event& operator=(Event&&) noexcept = default;

    template<class Function>
    Connection connect(Function&& function)
    {
        auto slot = std::make_shared<Slot>();
        slot->id = state_->nextId++;
        slot->callback = std::forward<Function>(function);
        state_->slots.push_back(slot);
        return Connection{state_, slot};
    }

    template<class Object, class Method>
    Connection connect(Object* object, Method method)
    {
        return connect([object, method](Args... args) {
            (object->*method)(std::forward<Args>(args)...);
        });
    }

    void emit(Args... args)
    {
        const auto snapshot = state_->slots;
        state_->dispatching = true;
        try {
            for (const auto& slot : snapshot) {
                if (slot->connected && slot->callback) {
                    slot->callback(args...);
                }
            }
        } catch (...) {
            state_->dispatching = false;
            state_->compact();
            throw;
        }
        state_->dispatching = false;
        state_->compact();
    }

    void operator()(Args... args) { emit(args...); }
    bool empty() const
    {
        return std::none_of(state_->slots.begin(), state_->slots.end(),
            [](const std::shared_ptr<Slot>& slot) { return slot->connected; });
    }
    std::size_t size() const
    {
        return static_cast<std::size_t>(std::count_if(
            state_->slots.begin(), state_->slots.end(),
            [](const std::shared_ptr<Slot>& slot) { return slot->connected; }));
    }
    void clear()
    {
        for (const auto& slot : state_->slots) {
            slot->connected = false;
        }
        state_->compact();
    }

private:
    std::shared_ptr<State> state_;
};

} // namespace peco
