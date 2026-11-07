/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** HierarchyPanel.h
*/

#ifndef HIERARCHYPANEL_H_
#define HIERARCHYPANEL_H_

#include <optional>
#include <string>

#include "editor/EditorContext.h"

namespace RType::Editor {
/**
 * @brief Lists the entities of the scene: select, create, rename and delete
 *        them.
 */
class HierarchyPanel {
public:
    static constexpr const char *TITLE = "Hierarchy";

    void draw(EditorContext &context);

    /** @brief Spawns an entity named "Entity <id>" and selects it. */
    static Entity createEntity(EditorContext &context);
    /** @brief Kills the entity right away and clears it from the selection. */
    static void deleteEntity(EditorContext &context, Entity entity);
    /** @brief Sets the Name of the entity, adding the component if needed. */
    static void rename(
        EditorContext &context, Entity entity, const std::string &name);
    /** @return The Name of the entity, or "Entity <id>" without one. */
    [[nodiscard]] static std::string label(Registry &registry, Entity entity);

private:
    void drawRenamePopup(EditorContext &context);

    std::optional<Entity> _renaming;
    std::string _renameBuffer;
};
} // namespace RType::Editor

#endif /* !HIERARCHYPANEL_H_ */
