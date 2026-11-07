/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** EditorContext.h
*/

#ifndef EDITORCONTEXT_H_
#define EDITORCONTEXT_H_

#include <filesystem>
#include <optional>

#include "ecs/Registry.h"
#include "library/AssetManager.h"
#include "library/InputManager.h"
#include "reflection/Reflection.h"
#include "render/Framebuffer.h"
#include "render/SceneRenderer.h"

namespace RType::Editor {
/** @brief State shared by every editor panel. */
struct EditorContext {
    Registry &registry;
    /** @brief The components the inspector can show and add. */
    const Reflection &reflection;
    /** @brief Lists the assets of AssetId fields. May be null. */
    const AssetManager *assets = nullptr;
    /** @brief The actions of the game, for Play mode. May be null. */
    const InputManager *input = nullptr;
    /** @brief Draws the scene in the viewport. Null in tests. */
    SceneRenderer *sceneRenderer = nullptr;
    /** @brief The image shown in the viewport. Null in tests. */
    Framebuffer *framebuffer = nullptr;
    /** @brief True while the scene plays: the panels must not edit it. */
    bool readOnly = false;
    /** @brief The file the scene was opened from or saved to, if any. */
    std::optional<std::filesystem::path> scenePath;
    /**
     * @brief The entity shown in the inspector, if any. Read it with
     *        selectedEntity(): it may have been killed since.
     */
    std::optional<Entity> selected;

    /** @brief The selection, or nothing if that entity is dead. */
    [[nodiscard]] std::optional<Entity> selectedEntity() const
    {
        if (selected && registry.isAlive(*selected))
            return selected;
        return std::nullopt;
    }

    /**
     * @brief Forgets the selection. Loading a scene must call it, since the
     *        selected entity does not exist in the new one.
     */
    void clearSelection()
    {
        selected.reset();
    }
};
} // namespace RType::Editor

#endif /* !EDITORCONTEXT_H_ */
