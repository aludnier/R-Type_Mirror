/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** EditorContext.h
*/

#ifndef EDITORCONTEXT_H_
#define EDITORCONTEXT_H_

#include <optional>

#include "ecs/Registry.h"
#include "library/AssetManager.h"
#include "reflection/Reflection.h"

namespace RType::Editor {
/** @brief State shared by every editor panel. */
struct EditorContext {
    Registry &registry;
    /** @brief The components the inspector can show and add. */
    const Reflection &reflection;
    /** @brief Lists the assets of AssetId fields. May be null. */
    const AssetManager *assets = nullptr;
    /** @brief True while the scene plays: the panels must not edit it. */
    bool readOnly = false;
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
