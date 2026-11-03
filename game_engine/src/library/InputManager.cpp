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
struct InputEntry {
    std::string id;
};

void from_json(const nlohmann::json &j, InputEntry &e)
{
    if (j.is_number_integer()) {
        e.id = InputManager::keyCodeId(j.get<InputManager::KeyCode>());
    } else if (j.is_string()) {
        e.id = j.get<std::string>();
        if (e.id.empty())
            throw std::invalid_argument("input names must not be empty");
    } else {
        throw std::invalid_argument(
            "an input must be a key code or a non-empty name");
    }
}

struct InputConfig {
    std::map<std::string, std::vector<InputEntry>> bindings;
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
    const auto config =
        RType::nlohamReader(configFile).convertJson<InputConfig>();

    std::vector<std::pair<Action, const std::vector<InputEntry> *>> resolved;
    for (const auto &[name, entries] : config.bindings) {
        const auto action = actionNames().find(name);
        if (action == actionNames().end())
            throw std::invalid_argument("unknown action '" + name + "'");
        resolved.emplace_back(action->second, &entries);
    }

    for (const auto &[action, entries] : resolved) {
        clearBindings(action);
        for (const auto &entry : *entries)
            bind(action, entry.id);
    }
}
std::string InputManager::keyCodeId(InputManager::KeyCode key)
{
    return "KEY_CODE_" + std::to_string(key);
}

void InputManager::bind(Action action, KeyCode key)
{
    bind(action, keyCodeId(key));
}

void InputManager::bind(Action action, const std::string &input)
{
    auto &inputs = _bindings[static_cast<std::size_t>(action)];
    if (std::ranges::find(inputs, input) == inputs.end()) {
        inputs.push_back(input);
    }
}

void InputManager::unbind(Action action, KeyCode key)
{
    unbind(action, keyCodeId(key));
}

void InputManager::unbind(Action action, std::string_view input)
{
    auto &inputs = _bindings[static_cast<std::size_t>(action)];
    std::erase(inputs, InputId(input));
}

void InputManager::clearBindings(Action action)
{
    _bindings[static_cast<std::size_t>(action)].clear();
}

void InputManager::setKeyState(KeyCode key, bool pressed)
{
    setInputState(keyCodeId(key), pressed);
}

void InputManager::setInputState(std::string_view input, bool pressed)
{
    if (pressed) {
        _pressed.insert(InputId(input));
    } else {
        _pressed.erase(InputId(input));
    }
}

bool InputManager::isActionDown(Action action) const
{
    const auto &inputs = _bindings[static_cast<std::size_t>(action)];
    return std::ranges::any_of(
        inputs, [this](const InputId &input) { return _pressed.contains(input); });
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
