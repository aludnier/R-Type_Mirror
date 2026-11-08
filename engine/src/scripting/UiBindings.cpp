/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Lua bindings for UI documents
*/

#include "scripting/UiBindings.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

#include "ui/UiManager.h"

sol::object RType::Scripting::UiBindingFunctions::load(
    sol::state &lua, UI::UiManager &manager)
{
    return sol::make_object(
        lua, [&manager](const std::string &name, const std::string &path) {
            manager.load(name, path);
        });
}

sol::object RType::Scripting::UiBindingFunctions::show(
    sol::state &lua, UI::UiManager &manager)
{
    return sol::make_object(
        lua, [&manager](const std::string &name) { manager.show(name); });
}

sol::object RType::Scripting::UiBindingFunctions::hide(
    sol::state &lua, UI::UiManager &manager)
{
    return sol::make_object(
        lua, [&manager](const std::string &name) { manager.hide(name); });
}

sol::object RType::Scripting::UiBindingFunctions::isVisible(
    sol::state &lua, UI::UiManager &manager)
{
    return sol::make_object(lua, [&manager](const std::string &name) {
        return manager.isVisible(name);
    });
}

sol::object RType::Scripting::UiBindingFunctions::getText(
    sol::state &lua, UI::UiManager &manager)
{
    return sol::make_object(lua,
        [&manager](const std::string &key) { return manager.getText(key); });
}

sol::object RType::Scripting::UiBindingFunctions::setText(
    sol::state &lua, UI::UiManager &manager)
{
    return sol::make_object(
        lua, [&manager](const std::string &key, const std::string &value) {
            manager.setText(key, value);
        });
}

sol::object RType::Scripting::UiBindingFunctions::getValue(
    sol::state &lua, UI::UiManager &manager)
{
    return sol::make_object(lua, [&lua, &manager](const std::string &key) {
        return std::visit(
            [&lua](const auto &value) { return sol::make_object(lua, value); },
            manager.getValue(key));
    });
}

sol::object RType::Scripting::UiBindingFunctions::setValue(
    sol::state &lua, UI::UiManager &manager)
{
    return sol::make_object(
        lua, [&manager](const std::string &key, const sol::object &value) {
            if (value.is<std::string>())
                manager.setValue(key, value.as<std::string>());
            else if (value.is<bool>())
                manager.setValue(key, value.as<bool>());
            else if (value.is<double>())
                manager.setValue(key, static_cast<float>(value.as<double>()));
            else
                throw std::invalid_argument(
                    "UI value must be a string, boolean or number");
        });
}

sol::object RType::Scripting::UiBindingFunctions::onClick(
    sol::state &lua, UI::UiManager &manager)
{
    return sol::make_object(lua,
        [&manager](const std::string &key, sol::protected_function callback) {
            manager.onClick(key, [callback = std::move(callback)] {
                const sol::protected_function_result result = callback();
                if (!result.valid()) {
                    const sol::error error = result;
                    throw std::runtime_error(error.what());
                }
            });
        });
}

sol::object RType::Scripting::UiBindingFunctions::takeClickedEvents(
    sol::state &lua, UI::UiManager &manager)
{
    return sol::make_object(lua, [&lua, &manager] {
        const auto events = manager.takeClickedEvents();
        sol::table result = lua.create_table(events.size(), 0);
        for (std::size_t i = 0; i < events.size(); ++i)
            result[i + 1] = events[i];
        return result;
    });
}

void RType::Scripting::bindUi(sol::state &lua, UI::UiManager &manager)
{
    sol::table ui = lua.create_table();
    for (const auto &[name, factory] : uiFunctions)
        ui[name] = factory(lua, manager);
    lua["ui"] = std::move(ui);
}
