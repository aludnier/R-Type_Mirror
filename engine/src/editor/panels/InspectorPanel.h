/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** InspectorPanel.h
*/

#ifndef INSPECTORPANEL_H_
#define INSPECTORPANEL_H_

#include "editor/EditorContext.h"

namespace RType::Editor {
/** @brief Shows the components of the selected entity. */
class InspectorPanel {
public:
    static constexpr const char *TITLE = "Inspector";

    void draw(EditorContext &context);
};
} // namespace RType::Editor

#endif /* !INSPECTORPANEL_H_ */
