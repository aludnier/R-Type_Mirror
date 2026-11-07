/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ComponentBinding.cpp
*/

#include "ComponentBinding.h"

#include <string>

template<>
sol::object RType::Scripting::ComponentOps<RType::ScriptComponent>::get(
    Registry &registry, entity_id_type id, lua_State *lua)
{
    auto &storage = registry.getComponents<ScriptComponent>();
    if (id >= storage.size() || !storage[id].has_value())
        return sol::make_object(lua, sol::lua_nil);
    sol::table paths = sol::state_view(lua).create_table();
    for (const auto &path : storage[id]->scripts)
        paths.add(path.string());
    return paths;
}

template<>
void RType::Scripting::ComponentOps<RType::ScriptComponent>::add(
    Registry &registry, entity_id_type id, const sol::object &value)
{
    ScriptComponent component;
    if (value.is<std::string>()) {
        component.scripts.emplace_back(value.as<std::string>());
    } else if (value.is<sol::table>()) {
        for (const auto &[_, path] : value.as<sol::table>()) {
            if (!path.is<std::string>())
                throw std::runtime_error("bad script path");
            component.scripts.emplace_back(path.as<std::string>());
        }
    } else {
        throw std::runtime_error("bad value for component");
    }
    registry.addComponent<ScriptComponent>(
        Registry::entityFromIndex(id), std::move(component));
}
