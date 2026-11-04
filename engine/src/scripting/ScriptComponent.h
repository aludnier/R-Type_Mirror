/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ScriptComponent.h
*/

#ifndef SCRIPTCOMPONENT_H_
#define SCRIPTCOMPONENT_H_

#include <cstddef>
#include <filesystem>
#include <vector>

namespace RType {
/**
 * @brief Lua scripts run on a single entity by the script system.
 *
 * Each script returns a table with optional `on_start(e)` (called once) and
 * `on_update(e, dt)` (called every frame) functions. Several scripts can run
 * on the same entity, in order.
 */
struct ScriptComponent {
    /** @brief Script paths, relative ones resolved from the script directory.
     */
    std::vector<std::filesystem::path> scripts;
    /** @brief Number of scripts whose on_start already ran. */
    std::size_t started = 0;
};
} // namespace RType

#endif /* !SCRIPTCOMPONENT_H_ */
