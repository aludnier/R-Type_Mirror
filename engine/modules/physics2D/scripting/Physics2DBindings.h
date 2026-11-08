/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Physics2DBindings.h
*/

#ifndef PHYSICS2D_BINDINGS_H
#define PHYSICS2D_BINDINGS_H

#include "scripting/ScriptEngine.h"

namespace RType::Physics2D {
/** @brief ModuleDescriptor::name of the physics2D module. */
inline constexpr const char *MODULE_NAME = "physics2d";

/**
 * @brief Makes the physics2D components usable from the scripts of
 *        @p scripts: Rigidbody2D, BoxCollider2D, CircleCollider2D,
 *        PhysicsSettings2D and CollisionEvents2D (the collisions of the
 *        frame), see docs/scripting.md.
 *
 * Compiled in the host, not in the module library: Lua is linked
 * statically, so the module must not touch the host's Lua state. Call it
 * once the module is loaded (PluginManager::isLoaded(MODULE_NAME)), as the
 * component storages must be registered before scripts use them.
 */
void bindScripting(ScriptEngine &scripts);
} // namespace RType::Physics2D

#endif // PHYSICS2D_BINDINGS_H
