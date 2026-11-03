/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ScriptSystem.h
*/

#ifndef SCRIPTSYSTEM_H_
#define SCRIPTSYSTEM_H_

#include "ScriptEngine.h"
#include "ecs/Registry.h"

namespace RType {
/**
 * @brief Adds the system running the scripts of @p engine.
 *
 * Every frame it runs the systems added with `world.add_system`, then the
 * ScriptComponent of each entity. It does nothing once @p engine is destroyed.
 */
void addScriptSystem(Registry &registry, ScriptEngine &engine);
} // namespace RType

#endif /* !SCRIPTSYSTEM_H_ */
