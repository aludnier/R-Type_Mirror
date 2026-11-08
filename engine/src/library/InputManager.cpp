/*
** EPITECH PROJECT, 2026
** RType
** File description:
** input manager
*/

#include "InputManager.h"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <map>
#include <ranges>
#include <stdexcept>

namespace {
constexpr std::string_view KEY_CODE_PREFIX = "KEY_CODE_";

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

std::string InputManager::keyCodeId(KeyCode key)
{
    return std::string(KEY_CODE_PREFIX) + std::to_string(key);
}

std::optional<InputManager::KeyCode> InputManager::keyCodeOf(
    std::string_view input)
{
    if (!input.starts_with(KEY_CODE_PREFIX))
        return std::nullopt;
    const std::string_view digits = input.substr(KEY_CODE_PREFIX.size());
    KeyCode key = 0;
    const auto [end, error] =
        std::from_chars(digits.data(), digits.data() + digits.size(), key);
    if (error != std::errc {} || end != digits.data() + digits.size())
        return std::nullopt;
    return key;
}

void InputManager::loadActions(const std::filesystem::path &configFile)
{
    const auto [bindings] =
        RType::nlohamReader(configFile).convertJson<InputConfig>();

    for (const auto &name : bindings | std::views::keys)
        addAction(name);
    loadConfigFile(configFile);
}

void InputManager::saveActions(const std::filesystem::path &configFile) const
{
    nlohmann::json bindings = nlohmann::json::object();

    for (const auto &[name, action] : _actionNames) {
        nlohmann::json inputs = nlohmann::json::array();
        for (const InputId &input : _bindings.at(action)) {
            if (const auto key = keyCodeOf(input))
                inputs.push_back(*key);
            else
                inputs.push_back(input);
        }
        bindings[name] = std::move(inputs);
    }
    std::ofstream output(configFile, std::ios::trunc);
    output << nlohmann::json {{"bindings", bindings}}.dump(4) << '\n';
    if (!output)
        throw std::runtime_error("cannot write " + configFile.string());
}

RType::ActionId InputManager::addAction(const std::string &name)
{
    if (const auto it = _actionNames.find(name); it != _actionNames.end())
        return it->second;
    RType::ActionMask used = 0;
    for (const RType::ActionId action : _actionNames | std::views::values)
        used |= RType::ActionMask {1} << action;
    for (std::size_t bit = 0; bit < RType::MAX_ACTIONS; ++bit) {
        if (((used >> bit) & 1U) == 0) {
            defineAction(name, static_cast<RType::ActionId>(bit));
            return static_cast<RType::ActionId>(bit);
        }
    }
    throw std::out_of_range(
        "too many actions: at most " + std::to_string(RType::MAX_ACTIONS));
}

void InputManager::removeAction(std::string_view name)
{
    const auto it = _actionNames.find(name);

    if (it == _actionNames.end())
        return;
    clearBindings(it->second);
    _actionNames.erase(it);
}

void InputManager::renameAction(std::string_view from, const std::string &to)
{
    const auto it = _actionNames.find(from);

    if (it == _actionNames.end())
        throw std::invalid_argument(
            "unknown action '" + std::string(from) + "'");
    if (from == to)
        return;
    if (_actionNames.contains(to))
        throw std::invalid_argument("action '" + to + "' already exists");
    const RType::ActionId action = it->second;
    _actionNames.erase(it);
    _actionNames.emplace(to, action);
}

const std::map<std::string, RType::ActionId, std::less<>> &
InputManager::actions() const
{
    return _actionNames;
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

const std::vector<std::string> &InputManager::bindings(
    RType::ActionId action) const
{
    return _bindings.at(action);
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

const std::unordered_set<std::string> &InputManager::pressedInputs() const
{
    return _pressed;
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
