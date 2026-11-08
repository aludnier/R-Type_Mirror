/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** InspectorPanel.cpp
*/

#include "InspectorPanel.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

#include <imgui.h>
#include <imgui_stdlib.h>

#include "math/Vector2.h"

namespace {
using RType::Editor::EditorContext;

constexpr const char *ADD_POPUP = "Add component";
constexpr const char *NO_ASSET = "None";

bool drawPathList(std::vector<std::filesystem::path> &paths)
{
    bool changed = false;
    std::optional<std::size_t> removed;

    for (std::size_t i = 0; i < paths.size(); ++i) {
        std::string text = paths[i].generic_string();

        ImGui::PushID(static_cast<int>(i));
        if (ImGui::InputText("##path", &text)) {
            paths[i] = text;
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("x"))
            removed = i;
        ImGui::PopID();
    }
    if (removed) {
        paths.erase(paths.begin() + static_cast<std::ptrdiff_t>(*removed));
        changed = true;
    }
    if (ImGui::SmallButton("+")) {
        paths.emplace_back();
        changed = true;
    }
    return changed;
}

bool drawAsset(
    const char *label, std::uint32_t &id, const EditorContext &context)
{
    if (context.assets == nullptr)
        return ImGui::DragScalar(label, ImGuiDataType_U32, &id);

    const auto current = context.assets->getAssetName(id);
    const std::string preview = id == 0 ? NO_ASSET
        : current                       ? *current
                                        : "<" + std::to_string(id) + ">";
    bool changed = false;

    if (!ImGui::BeginCombo(label, preview.c_str()))
        return false;
    if (ImGui::Selectable(NO_ASSET, id == 0)) {
        id = 0;
        changed = true;
    }
    for (const std::string &name : context.assets->names()) {
        const std::uint32_t assetId = context.assets->getAsset(name).id;

        if (ImGui::Selectable(name.c_str(), assetId == id)) {
            id = assetId;
            changed = true;
        }
    }
    ImGui::EndCombo();
    return changed;
}
} // namespace

namespace RType::Editor {
// Not static: the panel gets state once it needs some.
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void InspectorPanel::draw(EditorContext &context)
{
    ImGui::Begin(TITLE);
    const std::optional<Entity> entity = context.selectedEntity();

    if (!entity) {
        ImGui::TextDisabled("No entity selected");
        ImGui::End();
        return;
    }
    ImGui::BeginDisabled(context.readOnly);
    const ComponentInfo *removed = nullptr;
    for (const ComponentInfo &info : context.reflection.components()) {
        if (!info.has(context.registry, *entity))
            continue;
        ImGui::PushID(info.name.c_str());
        const bool open = ImGui::CollapsingHeader(
            info.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Remove component"))
                removed = &info;
            ImGui::EndPopup();
        }
        if (open) {
            // Got again for each component: adding one may move a storage.
            void *component = info.get(context.registry, *entity);
            for (const FieldInfo &field : info.fields)
                drawField(field, component, context);
            if (ImGui::SmallButton("Remove"))
                removed = &info;
        }
        ImGui::PopID();
    }
    // Removed after the loop, like the hierarchy deletes entities.
    if (removed != nullptr)
        removed->remove(context.registry, *entity);
    ImGui::Separator();
    drawAddComponent(context, *entity);
    ImGui::EndDisabled();
    ImGui::End();
}

bool InspectorPanel::drawField(
    const FieldInfo &field, void *component, const EditorContext &context)
{
    const char *label = field.name.c_str();

    switch (field.type) {
        case FieldType::Float:
            return ImGui::DragFloat(label, &field.as<float>(component), 0.1f);
        case FieldType::Int:
            return ImGui::DragInt(label, &field.as<int>(component));
        case FieldType::Bool:
            return ImGui::Checkbox(label, &field.as<bool>(component));
        case FieldType::String:
            return ImGui::InputText(label, &field.as<std::string>(component));
        case FieldType::Vector2:
            return ImGui::DragFloat2(
                label, &field.as<Vector2>(component).x, 0.5f);
        case FieldType::UInt8: {
            constexpr std::uint8_t min = 0;
            constexpr std::uint8_t max = 255;
            return ImGui::DragScalar(label,
                ImGuiDataType_U8,
                &field.as<std::uint8_t>(component),
                1.f,
                &min,
                &max);
        }
        case FieldType::UInt32:
            return ImGui::DragScalar(
                label, ImGuiDataType_U32, &field.as<std::uint32_t>(component));
        case FieldType::PathList: {
            ImGui::TextUnformatted(label);
            ImGui::PushID(label);
            const bool changed = drawPathList(
                field.as<std::vector<std::filesystem::path>>(component));
            ImGui::PopID();
            return changed;
        }
        case FieldType::AssetId:
            return drawAsset(
                label, field.as<std::uint32_t>(component), context);
    }
    return false;
}

std::vector<const ComponentInfo *> InspectorPanel::missingComponents(
    const EditorContext &context, Entity entity)
{
    std::vector<const ComponentInfo *> missing;

    for (const ComponentInfo &info : context.reflection.components())
        if (!info.has(context.registry, entity))
            missing.push_back(&info);
    return missing;
}

void InspectorPanel::drawAddComponent(EditorContext &context, Entity entity)
{
    if (ImGui::Button("Add component"))
        ImGui::OpenPopup(ADD_POPUP);
    if (!ImGui::BeginPopup(ADD_POPUP))
        return;
    for (const ComponentInfo *info : missingComponents(context, entity))
        if (ImGui::MenuItem(info->name.c_str()))
            info->add(context.registry, entity);
    ImGui::EndPopup();
}
} // namespace RType::Editor
