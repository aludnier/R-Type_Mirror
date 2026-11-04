/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** CoreBindings.h
*/

#ifndef COREBINDINGS_H_
#define COREBINDINGS_H_

#include <sol/sol.hpp>

namespace RType::Scripting {
/**
 * @brief Declares the Lua types of the engine core: Vector2, Transform2D,
 *        Health, Hitbox, Controllable and the Action enum.
 */
void bindCoreTypes(sol::state &lua);
} // namespace RType::Scripting

#endif /* !COREBINDINGS_H_ */
