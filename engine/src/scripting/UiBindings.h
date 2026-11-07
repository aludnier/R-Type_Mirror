/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Lua bindings for UI documents
*/

#ifndef UIBINDINGS_H_
#define UIBINDINGS_H_

#include <map>
#include <string>

#include <sol/sol.hpp>

namespace RType::UI {
class UiManager;
}

namespace RType::Scripting {

using UiFunctionFactory = sol::object (*)(sol::state &, UI::UiManager &);

namespace UiBindingFunctions {
    sol::object load(sol::state &lua, UI::UiManager &manager);
    sol::object show(sol::state &lua, UI::UiManager &manager);
    sol::object hide(sol::state &lua, UI::UiManager &manager);
    sol::object isVisible(sol::state &lua, UI::UiManager &manager);
    sol::object getText(sol::state &lua, UI::UiManager &manager);
    sol::object setText(sol::state &lua, UI::UiManager &manager);
    sol::object getValue(sol::state &lua, UI::UiManager &manager);
    sol::object setValue(sol::state &lua, UI::UiManager &manager);
    sol::object onClick(sol::state &lua, UI::UiManager &manager);
} // namespace UiBindingFunctions

inline const std::map<std::string, UiFunctionFactory> uiFunctions {
    {"load", UiBindingFunctions::load},
    {"show", UiBindingFunctions::show},
    {"hide", UiBindingFunctions::hide},
    {"is_visible", UiBindingFunctions::isVisible},
    {"get_text", UiBindingFunctions::getText},
    {"set_text", UiBindingFunctions::setText},
    {"get_value", UiBindingFunctions::getValue},
    {"set_value", UiBindingFunctions::setValue},
    {"on_click", UiBindingFunctions::onClick},
};

void bindUi(sol::state &lua, UI::UiManager &manager);

} // namespace RType::Scripting

#endif
