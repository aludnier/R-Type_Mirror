/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** GameBindings.h
*/

#ifndef GAME_GAMEBINDINGS_H
#define GAME_GAMEBINDINGS_H

#include "scripting/ScriptEngine.h"

namespace Game {
/**
 * @brief Makes the game components usable from the scripts of @p scripts:
 *        Enemy, Projectile, Player and ForcePod, see docs/scripting.md.
 *
 * Their storages must be registered in the registry before scripts use
 * them.
 */
void bindGameComponents(RType::ScriptEngine &scripts);
} // namespace Game

#endif // GAME_GAMEBINDINGS_H
