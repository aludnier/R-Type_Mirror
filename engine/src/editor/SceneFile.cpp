/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** SceneFile.cpp
*/

#include "SceneFile.h"

#include <exception>

#include "scene/Scene.h"

namespace RType::Editor::SceneFile {
void newScene(EditorContext &context)
{
    Scene::clear(context.registry);
    context.clearSelection();
    context.scenePath.reset();
}

std::optional<std::string> openScene(
    EditorContext &context, const std::filesystem::path &file)
{
    nlohmann::json scene;

    try {
        scene = Scene::readFile(file);
    } catch (const std::exception &error) {
        return error.what();
    }
    // Checked before clearing: any JSON file is not a scene, and it would
    // become the file the next Save overwrites.
    if (!scene.is_object() || !scene.contains("entities") ||
        !scene["entities"].is_array())
        return file.string() + " is not a scene: no \"entities\" array";
    // Without a manager, every asset keeps its default id.
    const AssetManager noAssets;
    newScene(context);
    Scene::fromJson(context.registry,
        context.reflection,
        scene,
        context.assets != nullptr ? *context.assets : noAssets);
    context.scenePath = file;
    return std::nullopt;
}

std::optional<std::string> saveScene(
    EditorContext &context, const std::filesystem::path &file)
{
    if (context.assets == nullptr)
        return "no AssetManager: sprite assets cannot be saved by name";
    try {
        Scene::saveToFile(
            context.registry, context.reflection, file, *context.assets);
    } catch (const std::exception &error) {
        return error.what();
    }
    context.scenePath = file;
    return std::nullopt;
}
} // namespace RType::Editor::SceneFile
