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
    if (std::ranges::find(keys, key) == keys.end()) {
        keys.push_back(key);
    }
}

void InputManager::unbind(Action action, KeyCode key)
{
    auto &keys = _bindings[static_cast<std::size_t>(action)];
    std::erase(keys, key);
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
    return std::ranges::any_of(
        keys, [this](KeyCode key) { return _pressed.contains(key); });
}

std::uint8_t InputManager::actionMask() const
{
    std::uint8_t mask = 0;
    for (std::size_t i = 0; i < actionCount; ++i) {
        if (isActionDown(static_cast<Action>(i))) {
            mask |= 1u << i;
        }
    }
    return mask;
}
