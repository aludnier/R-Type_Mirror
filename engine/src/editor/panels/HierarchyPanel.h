/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** HierarchyPanel.h
*/

#ifndef HIERARCHYPANEL_H_
#define HIERARCHYPANEL_H_

#include "editor/EditorContext.h"

namespace RType::Editor {
/** @brief Lists the entities of the scene. */
class HierarchyPanel {
public:
    static constexpr const char *TITLE = "Hierarchy";

    void draw(EditorContext &context);

private:
    const char *_title = TITLE;
};
} // namespace RType::Editor

#endif /* !HIERARCHYPANEL_H_ */
