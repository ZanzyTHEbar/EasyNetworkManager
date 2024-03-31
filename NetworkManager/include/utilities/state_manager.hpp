#pragma once
#include <algorithm>
#include <helpers/observer.hpp>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

/**
 * @brief StateManager is a singleton class that manages all states in the
 * program.
 * @note It is a singleton because it is used by many classes.
 */
template <typename StateVariant>
class StateManager : public Helpers::ISubject<StateVariant> {
   public:
    StateManager() = default;
    virtual ~StateManager() override = default;

    void setState(const std::string& key, const StateVariant& state) {
        states[key] = state;
        this->notify(key, state);
    }

    StateVariant getState(const std::string& key) const {
        auto it = states.find(key);
        if (it == states.end()) {
            log_e("State not found for key: %s", key.c_str());
            return {};
        }
        return it->second;
    }

   private:
    std::map<std::string, StateVariant> states;
};

// Define a generic function that applies a switch-case logic to an enum
// variant.
template <typename EnumType, typename VariantType, typename Func>
void updateWrapper(const VariantType& _variant, Func&& _switchCaseFunc) {
    std::visit(
        [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, EnumType>) {
                _switchCaseFunc(arg);
            }
        },
        _variant);
}