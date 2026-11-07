/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Scene.h
*/

#ifndef SCENE_H_
#define SCENE_H_

#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "ecs/Registry.h"
#include "library/AssetManager.h"
#include "reflection/Reflection.h"

/**
 * @brief Saves and loads the entities of a Registry as JSON.
 *
 * @code
 * {"entities": [{"name": "Player", "components": {
 *     "Transform2D": {"position": [120, 40], "scale": [1, 1], "rotation": 0},
 *     "Sprite": {"assetId": "player", "zIndex": 1},
 *     "Script": {"scripts": ["player.lua"]}}}]}
 * @endcode
 *
 * Only reflected components are saved. The Name component is the entity
 * "name", Vector2 is `[x, y]`, a PathList is an array of strings and an
 * AssetId is the asset name from the AssetManager. Entity ids are not
 * saved.
 *
 * Loading never throws on bad content: unknown components or fields, wrong
 * types and unknown assets are skipped with a warning, and what is missing
 * keeps its default value. The storage of a loaded component is registered
 * if needed.
 */
namespace RType::Scene {
/**
 * @param assets Turns asset ids into names. An unknown id is saved as null.
 * @note Entities waiting for killAllEntities() are not saved.
 */
[[nodiscard]] nlohmann::json toJson(Registry &registry,
    const Reflection &reflection, const AssetManager &assets);

/**
 * @brief Spawns the entities of @p scene. Existing entities are kept, call
 *        clear() first to replace the scene.
 * @param assets Turns asset names into ids. An unknown name keeps the default
 *        id.
 * @return The spawned entities, in file order.
 */
std::vector<Entity> fromJson(Registry &registry, const Reflection &reflection,
    const nlohmann::json &scene, const AssetManager &assets);

/** @throws nlohmann::json::exception if a string is not valid UTF-8. */
[[nodiscard]] std::string saveToString(Registry &registry,
    const Reflection &reflection, const AssetManager &assets);

/** @throws std::runtime_error if @p text is not valid JSON. */
std::vector<Entity> loadFromString(Registry &registry,
    const Reflection &reflection, const std::string &text,
    const AssetManager &assets);

/**
 * @brief Writes the scene to @p file. The scene is turned into text first, so
 *        an error there leaves an existing file untouched.
 * @throws std::runtime_error if the file cannot be written.
 * @throws nlohmann::json::exception if a string is not valid UTF-8.
 */
void saveToFile(Registry &registry, const Reflection &reflection,
    const std::filesystem::path &file, const AssetManager &assets);

/** @throws std::runtime_error if the file cannot be read or is not JSON. */
std::vector<Entity> loadFromFile(Registry &registry,
    const Reflection &reflection, const std::filesystem::path &file,
    const AssetManager &assets);

/** @brief Kills every entity and removes their components right away. */
void clear(Registry &registry);
} // namespace RType::Scene

#endif /* !SCENE_H_ */
