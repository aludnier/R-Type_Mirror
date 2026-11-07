/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** HierarchyPanel.cpp
*/

#include "HierarchyPanel.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include "components/Name.h"
#include "ecs/SparseArray.h"

namespace {
constexpr const char *RENAME_POPUP = "Rename entity";
} // namespace

namespace RType::Editor {
void HierarchyPanel::draw(EditorContext &context)
{
    ImGui::Begin(TITLE);
    ImGui::BeginDisabled(context.readOnly);
    if (ImGui::Button("Create entity"))
        createEntity(context);
    ImGui::EndDisabled();
    ImGui::Separator();

    const std::optional<Entity> selected = context.selectedEntity();
    std::optional<Entity> toDelete;
    bool openRename = false;
    for (const Entity &entity : context.registry.entities()) {
        const std::string name = label(context.registry, entity);

        ImGui::PushID(static_cast<int>(static_cast<entity_id_type>(entity)));
        if (ImGui::Selectable(name.c_str(), selected == entity))
            context.selected = entity;
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Rename", nullptr, false, !context.readOnly)) {
                _renaming = entity;
                _renameBuffer = name;
                openRename = true;
            }
            if (ImGui::MenuItem("Delete", nullptr, false, !context.readOnly))
                toDelete = entity;
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }
    // Changed after the loop: entities() must not change while iterating.
    if (toDelete)
        deleteEntity(context, *toDelete);
    if (openRename)
        ImGui::OpenPopup(RENAME_POPUP);
    drawRenamePopup(context);
    ImGui::End();
}

void HierarchyPanel::drawRenamePopup(EditorContext &context)
{
    if (!ImGui::BeginPopup(RENAME_POPUP))
        return;
    if (ImGui::IsWindowAppearing())
        ImGui::SetKeyboardFocusHere();
    const bool confirmed = ImGui::InputText(
        "##name", &_renameBuffer, ImGuiInputTextFlags_EnterReturnsTrue);
    if (confirmed || ImGui::Button("OK")) {
        if (_renaming && context.registry.isAlive(*_renaming))
            rename(context, *_renaming, _renameBuffer);
        _renaming.reset();
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

Entity HierarchyPanel::createEntity(EditorContext &context)
{
    const Entity entity = context.registry.spawnEntity();

    rename(context,
        entity,
        "Entity " + std::to_string(static_cast<entity_id_type>(entity)));
    context.selected = entity;
    return entity;
}

void HierarchyPanel::deleteEntity(EditorContext &context, Entity entity)
{
    if (context.selected == entity)
        context.clearSelection();
    context.registry.killEntity(entity);
    // Systems do not run in the editor: apply the kill now.
    context.registry.killAllEntities();
}

void HierarchyPanel::rename(
    EditorContext &context, Entity entity, const std::string &name)
{
    if (!context.registry.isRegistered<Name>())
        context.registry.registerComponent<Name>();
    context.registry.addComponent(entity, Name {name});
}

std::string HierarchyPanel::label(Registry &registry, Entity entity)
{
    if (registry.isRegistered<Name>()) {
        const auto &names = registry.getComponents<Name>();
        const auto id = static_cast<entity_id_type>(entity);

        if (id < names.size() && names[id].has_value())
            return names[id]->value;
    }
    return "Entity " + std::to_string(static_cast<entity_id_type>(entity));
}
} // namespace RType::Editor
