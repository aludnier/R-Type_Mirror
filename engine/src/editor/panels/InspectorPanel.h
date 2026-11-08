/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** InspectorPanel.h
*/

#ifndef INSPECTORPANEL_H_
#define INSPECTORPANEL_H_

#include <vector>

#include "editor/EditorContext.h"

namespace RType::Editor {
/**
 * @brief Edits the components of the selected entity through the reflection:
 *        it never names a component type.
 */
class InspectorPanel {
public:
    static constexpr const char *TITLE = "Inspector";

    void draw(EditorContext &context);

    /** @brief Draws the widget of one field. @return true if it changed. */
    static bool drawField(
        const FieldInfo &field, void *component, const EditorContext &context);

    /** @return The reflected components @p entity does not have yet. */
    [[nodiscard]] static std::vector<const ComponentInfo *> missingComponents(
        const EditorContext &context, Entity entity);

private:
    static void drawAddComponent(EditorContext &context, Entity entity);
};
} // namespace RType::Editor

#endif /* !INSPECTORPANEL_H_ */
