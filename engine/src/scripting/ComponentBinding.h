/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ComponentBinding.h
*/

#ifndef COMPONENTBINDING_H_
#define COMPONENTBINDING_H_

#include <concepts>
#include <cstddef>
#include <functional>
#include <stdexcept>

#include <sol/sol.hpp>

#include "ScriptComponent.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"

namespace RType::Scripting {
/**
 * @brief Type-erased access to one component storage, used by `world.*`.
 */
struct ComponentBinding {
    std::function<bool(Registry &, entity_id_type)> has;
    std::function<sol::object(Registry &, entity_id_type, lua_State *)> get;
    std::function<void(Registry &, entity_id_type, const sol::object &)> add;
    std::function<void(Registry &, entity_id_type)> remove;
    std::function<std::size_t(Registry &)> size;
};

/**
 * @brief The operations behind a ComponentBinding for a component type.
 *
 * `get` returns a reference to the component, `add` takes a value of the
 * component's Lua usertype, or nil for a default one. Specialize `get` / `add`
 * for a component that is not a usertype in Lua (see ScriptComponent).
 */
template<Component C>
struct ComponentOps {
    static bool has(Registry &registry, entity_id_type id)
    {
        if (!registry.isRegistered<C>())
            return false;
        auto &storage = registry.getComponents<C>();
        return id < storage.size() && storage[id].has_value();
    }

    static sol::object get(
        Registry &registry, entity_id_type id, lua_State *lua)
    {
        if (!registry.isRegistered<C>())
            return sol::make_object(lua, sol::lua_nil);
        auto &storage = registry.getComponents<C>();
        if (id >= storage.size() || !storage[id].has_value())
            return sol::make_object(lua, sol::lua_nil);
        return sol::make_object(lua, &storage[id].value());
    }

    static void add(
        Registry &registry, entity_id_type id, const sol::object &value)
    {
        if (!registry.isRegistered<C>())
            registry.registerComponent<C>();
        if (value.is<C>()) {
            registry.addComponent<C>(
                Registry::entityFromIndex(id), C(value.as<C>()));
            return;
        }
        if constexpr (std::default_initializable<C>) {
            if (!value.valid() || value.get_type() == sol::type::lua_nil) {
                registry.addComponent<C>(Registry::entityFromIndex(id), C {});
                return;
            }
        }
        throw std::runtime_error("bad value for component");
    }

    static void remove(Registry &registry, entity_id_type id)
    {
        if (!registry.isRegistered<C>())
            return;
        registry.removeComponent<C>(Registry::entityFromIndex(id));
    }

    static std::size_t size(Registry &registry)
    {
        if (!registry.isRegistered<C>())
            return 0;
        return registry.getComponents<C>().size();
    }
};

/** @brief Paths are exchanged as a Lua table of strings (a copy). */
template<>
sol::object ComponentOps<ScriptComponent>::get(
    Registry &registry, entity_id_type id, lua_State *lua);

/** @brief Accepts a path string or a table of path strings. */
template<>
void ComponentOps<ScriptComponent>::add(
    Registry &registry, entity_id_type id, const sol::object &value);

template<Component C>
ComponentBinding makeComponentBinding()
{
    return {
        .has = &ComponentOps<C>::has,
        .get = &ComponentOps<C>::get,
        .add = &ComponentOps<C>::add,
        .remove = &ComponentOps<C>::remove,
        .size = &ComponentOps<C>::size,
    };
}
} // namespace RType::Scripting

#endif /* !COMPONENTBINDING_H_ */
