#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>

#include <imgui.h>

#include "components/Health.h"
#include "components/Name.h"
#include "components/Sprite.h"
#include "components/Transform2D.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"
#include "editor/EditorContext.h"
#include "editor/panels/InspectorPanel.h"
#include "library/AssetManager.h"
#include "reflection/Reflection.h"
#include "scripting/ScriptComponent.h"

using RType::Editor::InspectorPanel;

namespace {
struct InspectorFixture {
    InspectorFixture()
    {
        RType::reflectCoreComponents(reflection);
        assets.addAsset(
            "player", "assets/player.png", AssetManager::AssetType::Texture);
    }

    RType::Registry registry;
    RType::Reflection reflection;
    AssetManager assets;
    RType::Editor::EditorContext context {
        .registry = registry, .reflection = reflection, .assets = &assets};
};

bool contains(const std::vector<const RType::ComponentInfo *> &components,
    const std::string &name)
{
    return std::ranges::any_of(components,
        [&](const RType::ComponentInfo *info) { return info->name == name; });
}

void drawFrames(InspectorPanel &panel, RType::Editor::EditorContext &context)
{
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(800.f, 600.f);
    io.DeltaTime = 1.f / 60.f;
    io.Fonts->Build();
    for (int frame = 0; frame < 2; ++frame) {
        ImGui::NewFrame();
        panel.draw(context);
        ImGui::Render();
    }
    ImGui::DestroyContext();
}
} // namespace

TEST_CASE("Inspector lists only the missing components", "[editor]")
{
    InspectorFixture fixture;
    const auto entity = fixture.registry.spawnEntity();
    const auto total = fixture.reflection.components().size();

    CHECK(InspectorPanel::missingComponents(fixture.context, entity).size() ==
        total);

    fixture.reflection.find("Health")->add(fixture.registry, entity);
    const auto missing =
        InspectorPanel::missingComponents(fixture.context, entity);

    CHECK(missing.size() == total - 1);
    CHECK_FALSE(contains(missing, "Health"));
    CHECK(contains(missing, "Transform2D"));

    fixture.reflection.find("Health")->remove(fixture.registry, entity);
    CHECK(contains(
        InspectorPanel::missingComponents(fixture.context, entity), "Health"));
}

TEST_CASE("Inspector fields edit the component", "[editor]")
{
    InspectorFixture fixture;
    const auto entity = fixture.registry.spawnEntity();
    RType::ComponentInfo &transform = *fixture.reflection.find("Transform2D");
    transform.add(fixture.registry, entity);

    void *component = transform.get(fixture.registry, entity);
    transform.fields[0].as<RType::Vector2>(component) = {3.f, 4.f};
    transform.fields[2].as<float>(component) = 1.5f;

    const auto &stored =
        fixture.registry.getComponents<RType::Transform2D>()[entity];
    CHECK(stored->position == RType::Vector2 {3.f, 4.f});
    CHECK(stored->rotation == 1.5f);
}

TEST_CASE("Inspector draws without a selection", "[editor]")
{
    InspectorFixture fixture;
    InspectorPanel panel;

    REQUIRE_NOTHROW(drawFrames(panel, fixture.context));
}

TEST_CASE("Inspector draws every field type", "[editor]")
{
    InspectorFixture fixture;
    InspectorPanel panel;
    const auto entity = fixture.registry.spawnEntity();
    for (const RType::ComponentInfo &info : fixture.reflection.components())
        info.add(fixture.registry, entity);
    fixture.registry.getComponents<RType::Sprite>()[entity]->assetId =
        fixture.assets.getAsset("player").id;
    fixture.registry.getComponents<RType::ScriptComponent>()[entity]
        ->scripts.emplace_back("player.lua");
    fixture.context.selected = entity;

    REQUIRE_NOTHROW(drawFrames(panel, fixture.context));
    fixture.context.readOnly = true;
    REQUIRE_NOTHROW(drawFrames(panel, fixture.context));
    fixture.context.assets = nullptr;
    REQUIRE_NOTHROW(drawFrames(panel, fixture.context));

    CHECK(fixture.registry.getComponents<RType::Sprite>()[entity]->assetId ==
        fixture.assets.getAsset("player").id);
    CHECK(InspectorPanel::missingComponents(fixture.context, entity).empty());
}

TEST_CASE("AssetManager lists asset names sorted", "[editor]")
{
    AssetManager assets;
    assets.addAsset("b", "b.png", AssetManager::AssetType::Texture);
    assets.addAsset("a", "a.png", AssetManager::AssetType::Texture);

    CHECK(assets.names() == std::vector<std::string> {"a", "b"});
}
