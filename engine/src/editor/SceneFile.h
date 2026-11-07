/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** SceneFile.h
*/

#ifndef SCENEFILE_H_
#define SCENEFILE_H_

#include <filesystem>
#include <optional>
#include <string>

#include "editor/EditorContext.h"

/** @brief New, Open and Save of the editor scene, used by the File menu. */
namespace RType::Editor::SceneFile {
/** @brief Removes every entity and forgets the selection and the path. */
void newScene(EditorContext &context);

/**
 * @brief Replaces the scene with @p file. The file is checked first: if it
 *        cannot be read or is not a scene, the current scene is kept.
 * @return The error, or nothing on success.
 */
std::optional<std::string> openScene(
    EditorContext &context, const std::filesystem::path &file);

/**
 * @brief Saves the scene to @p file and remembers the path.
 * @return The error, or nothing on success.
 */
std::optional<std::string> saveScene(
    EditorContext &context, const std::filesystem::path &file);
} // namespace RType::Editor::SceneFile

#endif /* !SCENEFILE_H_ */
