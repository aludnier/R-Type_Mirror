/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** RTypeScripting.h
*/

#ifndef RTYPESCRIPTING_H
#define RTYPESCRIPTING_H

#include <functional>

#include "GameProtocol.h"
#include "math/Vector2.h"
#include "scripting/ScriptEngine.h"

namespace RType {
/** @brief Spawns a replicated entity of @p kind at @p position. */
using SpawnFunction =
    std::function<void(Game::Protocol::EntityKind kind, Vector2 position)>;

/**
 * @brief Exposes the R-Type game to the server scripts: the Enemy
 *        component, and the `rtype` table (`rtype.spawn(kind, x, y)`,
 *        `rtype.SCREEN_WIDTH`, `rtype.SCREEN_HEIGHT`), see
 *        docs/scripting.md.
 * @param screenSize Size of the play area, in pixels.
 * @param spawn Called by `rtype.spawn`. Unknown kinds raise a Lua error.
 */
void bindRTypeScripting(
    ScriptEngine &scripts, Vector2 screenSize, SpawnFunction spawn);
} // namespace RType

#endif // RTYPESCRIPTING_H
