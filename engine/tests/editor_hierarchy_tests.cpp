#include <catch2/catch_test_macros.hpp>

#include <imgui.h>

#include "components/Name.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"
#include "editor/EditorContext.h"
#include "editor/panels/HierarchyPanel.h"
#include "reflection/Reflection.h"

using RType::Editor::HierarchyPanel;

TEST_CASE("Hierarchy creates a named entity and selects it", "[editor]")
{
    RType::Registry registry;
    RType::Reflection reflection;
    RType::Editor::EditorContext context {
        .registry = registry, .reflection = reflection};

    const auto entity = HierarchyPanel::createEntity(context);

    REQUIRE(registry.entities().size() == 1);
    CHECK(context.selectedEntity() == entity);
    CHECK(registry.isRegistered<RType::Name>());
    CHECK(HierarchyPanel::label(registry, entity) == "Entity 0");
}

TEST_CASE("Hierarchy deletes an entity and clears the selection", "[editor]")
{
    RType::Registry registry;
    RType::Reflection reflection;
    RType::Editor::EditorContext context {
        .registry = registry, .reflection = reflection};
    const auto kept = HierarchyPanel::createEntity(context);
    const auto deleted = HierarchyPanel::createEntity(context);

    HierarchyPanel::deleteEntity(context, deleted);

    CHECK(registry.entities() == std::vector {kept});
    CHECK_FALSE(context.selected.has_value());
    CHECK_FALSE(registry.getComponents<RType::Name>()[deleted].has_value());

    context.selected = kept;
    HierarchyPanel::deleteEntity(
        context, HierarchyPanel::createEntity(context));
    CHECK_FALSE(context.selected.has_value());
}

TEST_CASE("Deleting another entity keeps the selection", "[editor]")
{
    RType::Registry registry;
    RType::Reflection reflection;
    RType::Editor::EditorContext context {
        .registry = registry, .reflection = reflection};
    const auto other = HierarchyPanel::createEntity(context);
    const auto selected = HierarchyPanel::createEntity(context);

    HierarchyPanel::deleteEntity(context, other);

    CHECK(context.selectedEntity() == selected);
}

TEST_CASE("Selection ignores dead entities", "[editor]")
{
    RType::Registry registry;
    RType::Reflection reflection;
    RType::Editor::EditorContext context {
        .registry = registry, .reflection = reflection};
    const auto entity = registry.spawnEntity();

    context.selected = entity;
    CHECK(context.selectedEntity() == entity);
    registry.killEntity(entity);
    registry.killAllEntities();
    CHECK_FALSE(context.selectedEntity().has_value());

    context.clearSelection();
    CHECK_FALSE(context.selected.has_value());
}

TEST_CASE("Hierarchy renames entities", "[editor]")
{
    RType::Registry registry;
    RType::Reflection reflection;
    RType::Editor::EditorContext context {
        .registry = registry, .reflection = reflection};
    const auto unnamed = registry.spawnEntity();

    CHECK(HierarchyPanel::label(registry, unnamed) == "Entity 0");
    HierarchyPanel::rename(context, unnamed, "Player");

    CHECK(HierarchyPanel::label(registry, unnamed) == "Player");
    CHECK(registry.getComponents<RType::Name>()[unnamed]->value == "Player");
}

TEST_CASE("Hierarchy panel draws the entities", "[editor]")
{
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(800.f, 600.f);
    io.DeltaTime = 1.f / 60.f;
    io.Fonts->Build();
    RType::Registry registry;
    RType::Reflection reflection;
    RType::Editor::EditorContext context {
        .registry = registry, .reflection = reflection};
    HierarchyPanel panel;
    HierarchyPanel::createEntity(context);
    registry.spawnEntity();

    for (int frame = 0; frame < 2; ++frame) {
        ImGui::NewFrame();
        panel.draw(context);
        ImGui::Render();
    }

    CHECK(ImGui::GetDrawData()->TotalVtxCount > 0);
    ImGui::DestroyContext();
}
