/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** InspectorPanel.cpp
*/

#include "InspectorPanel.h"

#include <imgui.h>

namespace RType::Editor {
// Not static: the panel gets state once it is filled.
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void InspectorPanel::draw(EditorContext & /*context*/)
{
    ImGui::Begin(TITLE);
    ImGui::End();
}
} // namespace RType::Editor
