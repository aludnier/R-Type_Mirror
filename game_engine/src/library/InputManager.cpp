/*
** EPITECH PROJECT, 2026
** RType
** File description:
** input manager
*/

#include "InputManager.h"
#include <algorithm>
#include <map>
#include <stdexcept>

namespace {
struct InputConfig {
    std::map<std::string, std::vector<InputManager::KeyCode>> bindings;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(InputConfig, bindings)

const std::map<std::string, Action> &actionNames()
{
    static const std::map<std::string, Action> actions = {
        {"MoveUp", Action::MoveUp},
        {"MoveDown", Action::MoveDown},
        {"MoveLeft", Action::MoveLeft},
        {"MoveRight", Action::MoveRight},
        {"Shoot", Action::Shoot}};
    return actions;
}
} // namespace

InputManager::InputManager(const std::filesystem::path &configFile)
{
    loadConfigFile(configFile);
}

void InputManager::loadConfigFile(const std::filesystem::path &configFile)
{
    if (!_reader.readFile(configFile))
        throw std::runtime_error(
            "[InputManager] cannot read " + configFile.string());
    const auto config = _reader.convertJson<InputConfig>();

    std::vector<std::pair<Action, std::vector<KeyCode>>> bindings;
    bindings.reserve(config.bindings.size());
    for (const auto &[name, keys] : config.bindings) {
        const auto action = actionNames().find(name);
        if (action == actionNames().end())
            throw std::invalid_argument("unknown action '" + name + "'");
        bindings.emplace_back(action->second, keys);
    }

    for (const auto &[action, keys] : bindings) {
        clearBindings(action);
        for (const auto key : keys)
            bind(action, key);
    }
}

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
