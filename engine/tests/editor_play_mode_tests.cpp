#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <system_error>
#include <utility>
#include <vector>

#include "components/Controllable.h"
#include "components/Name.h"
#include "components/Transform2D.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"
#include "editor/EditorContext.h"
#include "editor/PlayMode.h"
#include "editor/panels/HierarchyPanel.h"
#include "library/AssetManager.h"
#include "library/InputManager.h"
#include "reflection/Reflection.h"
#include "scripting/ScriptComponent.h"

using RType::Editor::HierarchyPanel;

namespace {
constexpr const char *MOVE_SCRIPT = R"(
return {
    on_update = function(id, dt)
        if not world.has(id, "Transform2D") then
            world.add(id, "Transform2D", Transform2D.new())
        end
        local transform = world.get(id, "Transform2D")
        transform.position = transform.position + Vector2.new(10, 0)
        world.spawn()
    end,
}
)";

constexpr const char *INPUT_SCRIPT = R"(
return {
    on_update = function(id, dt)
        local transform = world.get(id, "Transform2D")
        if world.get(id, "Controllable"):is_active(Action.Right) then
            transform.position = transform.position + Vector2.new(5, 0)
        end
    end,
}
)";

struct PlayFixture {
    PlayFixture()
    {
        RType::reflectCoreComponents(reflection);
        std::ofstream(script) << MOVE_SCRIPT;
    }

    ~PlayFixture()
    {
        std::error_code error;
        std::filesystem::remove(script, error);
    }

    PlayFixture(const PlayFixture &) = delete;
    PlayFixture &operator=(const PlayFixture &) = delete;

    /** @brief An entity at the origin, with the script attached. */
    RType::Entity scripted()
    {
        const auto entity = HierarchyPanel::createEntity(context);

        reflection.find("Transform2D")->add(registry, entity);
        attach(entity);
        return entity;
    }

    void attach(RType::Entity entity)
    {
        reflection.find("Script")->add(registry, entity);
        registry.getComponents<RType::ScriptComponent>()[entity]
            ->scripts.emplace_back(script);
    }

    RType::Registry registry;
    RType::Reflection reflection;
    AssetManager assets;
    RType::Editor::EditorContext context {
        .registry = registry, .reflection = reflection, .assets = &assets};
    RType::Editor::PlayMode play;
    std::filesystem::path script =
        std::filesystem::temp_directory_path() / "rtype_editor_play.lua";
};
} // namespace

TEST_CASE("Play runs the scripts and locks the panels", "[editor]")
{
    PlayFixture fixture;
    const auto entity = fixture.scripted();

    fixture.play.play(fixture.context);
    CHECK(fixture.play.isPlaying());
    CHECK(fixture.context.readOnly);

    fixture.play.update(fixture.context, 1.f / 60.f);
    fixture.play.update(fixture.context, 1.f / 60.f);

    const auto &transforms =
        fixture.registry.getComponents<RType::Transform2D>();
    CHECK(transforms[entity]->position.x == 20.f);
    CHECK(fixture.registry.entities().size() == 3);
}

TEST_CASE("Stop restores the scene", "[editor]")
{
    PlayFixture fixture;
    const auto entity = fixture.scripted();
    HierarchyPanel::rename(fixture.context, entity, "Player");

    fixture.play.play(fixture.context);
    fixture.play.update(fixture.context, 1.f / 60.f);
    fixture.play.stop(fixture.context);

    CHECK_FALSE(fixture.play.isPlaying());
    CHECK_FALSE(fixture.context.readOnly);
    CHECK_FALSE(fixture.context.selected.has_value());
    REQUIRE(fixture.registry.entities().size() == 1);
    const auto restored = fixture.registry.entities().front();
    const auto &transforms =
        fixture.registry.getComponents<RType::Transform2D>();
    CHECK(transforms[restored]->position.x == 0.f);
    CHECK(HierarchyPanel::label(fixture.registry, restored) == "Player");
    CHECK(fixture.registry.getComponents<RType::ScriptComponent>()[restored]
              ->scripts == std::vector {fixture.script});
}

TEST_CASE("The scene can be played again after Stop", "[editor]")
{
    PlayFixture fixture;
    fixture.scripted();

    fixture.play.play(fixture.context);
    fixture.play.update(fixture.context, 1.f / 60.f);
    fixture.play.stop(fixture.context);

    fixture.play.play(fixture.context);
    fixture.play.update(fixture.context, 1.f / 60.f);

    const auto entity = fixture.registry.entities().front();
    const auto &transforms =
        fixture.registry.getComponents<RType::Transform2D>();
    CHECK(transforms[entity]->position.x == 10.f);
}

TEST_CASE("Scripts can use components the scene does not have", "[editor]")
{
    PlayFixture fixture;
    const auto entity = HierarchyPanel::createEntity(fixture.context);
    fixture.attach(entity);
    REQUIRE_FALSE(fixture.registry.isRegistered<RType::Transform2D>());

    fixture.play.play(fixture.context);
    fixture.play.update(fixture.context, 1.f / 60.f);

    CHECK(fixture.registry.getComponents<RType::Transform2D>()[entity]
              ->position.x == 10.f);
}

TEST_CASE("Play needs an AssetManager", "[editor]")
{
    PlayFixture fixture;
    fixture.context.assets = nullptr;

    fixture.play.play(fixture.context);

    CHECK_FALSE(fixture.play.isPlaying());
    CHECK_FALSE(fixture.context.readOnly);
}

TEST_CASE("Update and Stop do nothing when not playing", "[editor]")
{
    PlayFixture fixture;
    const auto entity = fixture.scripted();

    fixture.play.update(fixture.context, 1.f / 60.f);
    fixture.play.stop(fixture.context);

    CHECK(fixture.registry.isAlive(entity));
    CHECK(fixture.context.selectedEntity() == entity);
}

TEST_CASE("Scripts read the actions during Play", "[editor]")
{
    PlayFixture fixture;
    InputManager input;
    constexpr RType::ActionId right = 3;
    constexpr RType::ActionMask pressed = RType::ActionMask {1} << right;
    input.defineAction("Right", right);
    fixture.context.input = &input;
    std::ofstream(fixture.script) << INPUT_SCRIPT;
    const auto entity = fixture.scripted();
    fixture.reflection.find("Controllable")->add(fixture.registry, entity);

    fixture.play.play(fixture.context);
    const auto &transforms =
        fixture.registry.getComponents<RType::Transform2D>();

    fixture.play.update(fixture.context, 1.f / 60.f, 0);
    CHECK(transforms[entity]->position.x == 0.f);
    fixture.play.update(fixture.context, 1.f / 60.f, pressed);
    CHECK(transforms[entity]->position.x == 5.f);
    fixture.play.update(fixture.context, 1.f / 60.f, 0);
    CHECK(transforms[entity]->position.x == 5.f);
}
