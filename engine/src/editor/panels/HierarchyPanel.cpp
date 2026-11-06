/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** HierarchyPanel.cpp
*/

#include "HierarchyPanel.h"

#include <imgui.h>

namespace RType::Editor {
void HierarchyPanel::draw(EditorContext & /*context*/)
{
    ImGui::Begin(_title);
    ImGui::End();
}
} // namespace RType::Editor
