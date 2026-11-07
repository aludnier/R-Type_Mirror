/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** EditorShell.h
*/

#ifndef EDITORSHELL_H_
#define EDITORSHELL_H_

#include "editor/EditorContext.h"
#include "editor/FileMenu.h"
#include "editor/panels/HierarchyPanel.h"
#include "editor/panels/InspectorPanel.h"
#include "editor/panels/ViewportPanel.h"

namespace RType::Editor {
/**
 * @brief The editor window: main menu, a dockspace over the whole window and
 *        the panels.
 *
 * Without an imgui.ini, the panels start with Hierarchy on the left,
 * Inspector on the right and Viewport in the middle.
 */
class EditorShell {
public:
    /** @brief Draws one frame. Call it between ImGuiLayer begin/endFrame. */
    void draw(EditorContext &context);

private:
    void drawMenuBar(EditorContext &context);

    FileMenu _fileMenu;
    HierarchyPanel _hierarchy;
    ViewportPanel _viewport;
    InspectorPanel _inspector;
};
} // namespace RType::Editor

#endif /* !EDITORSHELL_H_ */
