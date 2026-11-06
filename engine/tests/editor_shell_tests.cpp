#include <catch2/catch_test_macros.hpp>

#include <imgui.h>
#include <imgui_internal.h>

#include "ecs/Registry.h"
#include "editor/EditorContext.h"
#include "editor/EditorShell.h"

namespace {
/** @brief ImGui without a window or a renderer, like a test backend. */
class HeadlessImGui {
public:
    HeadlessImGui()
    {
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.IniFilename = nullptr;
        io.DisplaySize = ImVec2(1280.f, 720.f);
        io.DeltaTime = 1.f / 60.f;
        io.Fonts->Build();
    }

    ~HeadlessImGui()
    {
        ImGui::DestroyContext();
    }

    HeadlessImGui(const HeadlessImGui &) = delete;
    HeadlessImGui &operator=(const HeadlessImGui &) = delete;
};

void drawFrame(
    RType::Editor::EditorShell &shell, RType::Editor::EditorContext &context)
{
    ImGui::NewFrame();
    shell.draw(context);
    ImGui::Render();
}
} // namespace

TEST_CASE("Editor shell docks the three panels", "[editor]")
{
    HeadlessImGui imgui;
    RType::Registry registry;
    RType::Editor::EditorContext context {.registry = registry};
    RType::Editor::EditorShell shell;

    for (int frame = 0; frame < 3; ++frame)
        drawFrame(shell, context);

    ImGuiWindow *hierarchy = ImGui::FindWindowByName("Hierarchy");
    ImGuiWindow *viewport = ImGui::FindWindowByName("Viewport");
    ImGuiWindow *inspector = ImGui::FindWindowByName("Inspector");
    REQUIRE(hierarchy != nullptr);
    REQUIRE(viewport != nullptr);
    REQUIRE(inspector != nullptr);
    CHECK(hierarchy->DockIsActive);
    CHECK(viewport->DockIsActive);
    CHECK(inspector->DockIsActive);
    CHECK(hierarchy->Pos.x < viewport->Pos.x);
    CHECK(viewport->Pos.x < inspector->Pos.x);
    CHECK(viewport->Size.x > hierarchy->Size.x);
}

TEST_CASE("Editor context holds one selection", "[editor]")
{
    RType::Registry registry;
    RType::Editor::EditorContext context {.registry = registry};

    CHECK_FALSE(context.selected.has_value());
    context.selected = registry.spawnEntity();
    CHECK(context.selected == registry.entities().front());
}
