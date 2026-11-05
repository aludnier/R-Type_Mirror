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
} // namespace

void InputManager::defineAction(const std::string &name, RType::ActionId action)
{
    if (action >= RType::MAX_ACTIONS)
        throw std::out_of_range("action '" + name + "' does not fit the mask");
    _actionNames.insert_or_assign(name, action);
}

void InputManager::loadConfigFile(const std::filesystem::path &configFile)
{
    const auto [bindings] =
        RType::nlohamReader(configFile).convertJson<InputConfig>();

    std::vector<std::pair<RType::ActionId, const std::vector<InputEntry> *>>
        resolved;
    for (const auto &[name, entries] : bindings) {
        const auto action = _actionNames.find(name);
        if (action == _actionNames.end())
            throw std::invalid_argument("unknown action '" + name + "'");
        resolved.emplace_back(action->second, &entries);
    }

    for (const auto &[action, entries] : resolved) {
        clearBindings(action);
        for (const auto &[id] : *entries)
            bind(action, id);
    }
}

std::string InputManager::keyCodeId(InputManager::KeyCode key)
{
    return "KEY_CODE_" + std::to_string(key);
}

void InputManager::bind(RType::ActionId action, KeyCode key)
{
    bind(action, keyCodeId(key));
}

void InputManager::bind(RType::ActionId action, const std::string &input)
{
    auto &inputs = _bindings.at(action);
    if (std::ranges::find(inputs, input) == inputs.end()) {
        inputs.push_back(input);
    }
}

void InputManager::unbind(RType::ActionId action, KeyCode key)
{
    unbind(action, keyCodeId(key));
}

void InputManager::unbind(RType::ActionId action, std::string_view input)
{
    auto &inputs = _bindings.at(action);
    std::erase(inputs, InputId(input));
}

void InputManager::clearBindings(RType::ActionId action)
{
    _bindings.at(action).clear();
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

bool InputManager::isActionDown(RType::ActionId action) const
{
    const auto &inputs = _bindings.at(action);
    return std::ranges::any_of(inputs,
        [this](const InputId &input) { return _pressed.contains(input); });
}

RType::ActionMask InputManager::actionMask() const
{
    RType::ActionMask mask = 0;
    for (std::size_t i = 0; i < RType::MAX_ACTIONS; ++i) {
        if (isActionDown(static_cast<RType::ActionId>(i))) {
            mask |= 1u << i;
        }
    }
    return mask;
}
