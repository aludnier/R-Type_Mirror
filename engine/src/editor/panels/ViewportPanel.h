/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ViewportPanel.h
*/

#ifndef VIEWPORTPANEL_H_
#define VIEWPORTPANEL_H_

#include "editor/EditorContext.h"

namespace RType::Editor {
/** @brief Shows the scene, drawn in a framebuffer. */
class ViewportPanel {
public:
    static constexpr const char *TITLE = "Viewport";

    void draw(EditorContext &context);
};
} // namespace RType::Editor

#endif /* !VIEWPORTPANEL_H_ */
