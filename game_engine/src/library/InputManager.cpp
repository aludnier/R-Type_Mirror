/*
** EPITECH PROJECT, 2026
** RType
** File description:
** input manager
*/

#include "InputManager.h"
#include <algorithm>

void InputManager::bind(Action action, KeyCode key)
{
    auto &keys = _bindings[static_cast<std::size_t>(action)];
    if (std::find(keys.begin(), keys.end(), key) == keys.end()) {
        keys.push_back(key);
    }
}

void InputManager::unbind(Action action, KeyCode key)
{
    auto &keys = _bindings[static_cast<std::size_t>(action)];
    keys.erase(std::remove(keys.begin(), keys.end(), key), keys.end());
}

void InputManager::clearBindings(Action action)
{
    _bindings[static_cast<std::size_t>(action)].clear();
}

void InputManager::setKeyState(KeyCode key, bool pressed)
{
    if (pressed) {
        _pressed.insert(key);
    } else {
        _pressed.erase(key);
    }
}

bool InputManager::isActionDown(Action action) const
{
    const auto &keys = _bindings[static_cast<std::size_t>(action)];
    return std::any_of(keys.begin(), keys.end(),
                       [this](KeyCode key) { return _pressed.count(key) > 0; });
}

std::uint8_t InputManager::actionMask() const
{
    std::uint8_t mask = 0;
    for (std::size_t i = 0; i < actionCount; ++i) {
        if (isActionDown(static_cast<Action>(i))) {
            mask |= (1u << i);
        }
    }
    return mask;
}
