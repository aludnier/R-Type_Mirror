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
#include "editor/PlayMode.h"
#include "editor/panels/HierarchyPanel.h"
#include "editor/panels/InputPanel.h"
#include "editor/panels/InspectorPanel.h"
#include "editor/panels/ViewportPanel.h"

namespace RType::Editor {
/**
 * @brief The editor window: main menu, a dockspace over the whole window and
 *        the panels.
 *
 * Without an imgui.ini, the panels start with Hierarchy on the left,
 * Inspector and Input as tabs on the right and Viewport in the middle.
 */
class EditorShell {
public:
    /** @brief Draws one frame. Call it between ImGuiLayer begin/endFrame. */
    void draw(EditorContext &context);
    /** @brief Runs one step of the scene while it plays. */
    void update(EditorContext &context, float dt);
    /** @return The active actions, none while ImGui uses the keyboard. */
    [[nodiscard]] static ActionMask gameActions(const EditorContext &context);

private:
    void drawMenuBar(EditorContext &context);

    FileMenu _fileMenu;
    PlayMode _playMode;
    HierarchyPanel _hierarchy;
    ViewportPanel _viewport;
    InspectorPanel _inspector;
    InputPanel _input;
};
} // namespace RType::Editor

#endif /* !EDITORSHELL_H_ */
