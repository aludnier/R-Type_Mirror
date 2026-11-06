#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>
#include <vector>

#include "components/Health.h"
#include "components/Name.h"
#include "components/Sprite.h"
#include "components/Transform2D.h"
#include "ecs/Registry.h"
#include "reflection/Reflection.h"
#include "scripting/ScriptComponent.h"

TEST_CASE("Reflection registers components in order", "[reflection]")
{
    RType::Reflection reflection;

    reflection.reflect<RType::Health>("Health")
        .field("current", &RType::Health::current)
        .field("max", &RType::Health::max);
    reflection.reflect<RType::Name>("Name").field("value", &RType::Name::value);

    REQUIRE(reflection.components().size() == 2);
    CHECK(reflection.components()[0].name == "Health");
    CHECK(reflection.components()[1].name == "Name");
    const auto &fields = reflection.components()[0].fields;
    REQUIRE(fields.size() == 2);
    CHECK(fields[0].name == "current");
    CHECK(fields[0].type == RType::FieldType::Int);
    CHECK(fields[1].name == "max");
}

TEST_CASE("Reflection finds components by name", "[reflection]")
{
    RType::Reflection reflection;
    RType::reflectCoreComponents(reflection);

    const RType::ComponentInfo *transform = reflection.find("Transform2D");
    REQUIRE(transform != nullptr);
    CHECK(transform->name == "Transform2D");
    REQUIRE(transform->fields.size() == 3);
    CHECK(transform->fields[0].type == RType::FieldType::Vector2);
    CHECK(transform->fields[2].type == RType::FieldType::Float);
    REQUIRE(reflection.find("Sprite") != nullptr);
    CHECK(
        reflection.find("Sprite")->fields[0].type == RType::FieldType::UInt32);
    CHECK(reflection.find("Missing") == nullptr);
}

TEST_CASE("Reflecting a name twice replaces the entry", "[reflection]")
{
    RType::Reflection reflection;

    reflection.reflect<RType::Health>("Health").field(
        "current", &RType::Health::current);
    reflection.reflect<RType::Health>("Health").field(
        "max", &RType::Health::max);

    REQUIRE(reflection.components().size() == 1);
    REQUIRE(reflection.components()[0].fields.size() == 1);
    CHECK(reflection.components()[0].fields[0].name == "max");
}

TEST_CASE("ComponentInfo adds, gets and removes components", "[reflection]")
{
    RType::Registry registry;
    registry.registerComponent<RType::Health>();
    RType::Reflection reflection;
    RType::reflectCoreComponents(reflection);
    auto entity = registry.spawnEntity();
    RType::ComponentInfo &health = *reflection.find("Health");

    CHECK_FALSE(health.has(registry, entity));
    CHECK(health.get(registry, entity) == nullptr);

    health.add(registry, entity);
    REQUIRE(health.has(registry, entity));
    CHECK(health.get(registry, entity) ==
        &registry.getComponents<RType::Health>()[entity].value());

    health.remove(registry, entity);
    CHECK_FALSE(health.has(registry, entity));
}

TEST_CASE("ComponentInfo ignores unregistered storages", "[reflection]")
{
    RType::Registry registry;
    RType::Reflection reflection;
    RType::reflectCoreComponents(reflection);
    auto entity = registry.spawnEntity();

    for (RType::ComponentInfo &component : reflection.components()) {
        CHECK_FALSE(component.has(registry, entity));
        CHECK(component.get(registry, entity) == nullptr);
    }
}

TEST_CASE("FieldInfo reads and writes component fields", "[reflection]")
{
    RType::Registry registry;
    registry.registerComponent<RType::Transform2D>();
    registry.registerComponent<RType::Name>();
    RType::Reflection reflection;
    RType::reflectCoreComponents(reflection);
    auto entity = registry.spawnEntity();
    registry.addComponent(
        entity, RType::Transform2D {RType::Vector2 {1.f, 2.f}});
    registry.addComponent(entity, RType::Name {"Player"});

    RType::ComponentInfo &transform = *reflection.find("Transform2D");
    void *component = transform.get(registry, entity);
    const RType::FieldInfo &position = transform.fields[0];
    CHECK(position.as<RType::Vector2>(component) == RType::Vector2 {1.f, 2.f});
    position.as<RType::Vector2>(component) = {5.f, 6.f};
    *static_cast<float *>(transform.fields[2].get(component)) = 0.5f;

    const auto &stored = registry.getComponents<RType::Transform2D>()[entity];
    CHECK(stored->position == RType::Vector2 {5.f, 6.f});
    CHECK(stored->rotation == 0.5f);

    RType::ComponentInfo &name = *reflection.find("Name");
    name.fields[0].as<std::string>(name.get(registry, entity)) = "Boss";
    CHECK(registry.getComponents<RType::Name>()[entity]->value == "Boss");
}

TEST_CASE("Script component exposes its paths only", "[reflection]")
{
    RType::Reflection reflection;
    RType::reflectCoreComponents(reflection);

    const RType::ComponentInfo *script = reflection.find("Script");
    REQUIRE(script != nullptr);
    REQUIRE(script->fields.size() == 1);
    CHECK(script->fields[0].name == "scripts");
    CHECK(script->fields[0].type == RType::FieldType::PathList);
}

TEST_CASE("FieldInfo edits the script paths", "[reflection]")
{
    RType::Registry registry;
    registry.registerComponent<RType::ScriptComponent>();
    RType::Reflection reflection;
    RType::reflectCoreComponents(reflection);
    auto entity = registry.spawnEntity();
    RType::ComponentInfo &script = *reflection.find("Script");

    script.add(registry, entity);
    auto &paths = script.fields[0].as<std::vector<std::filesystem::path>>(
        script.get(registry, entity));
    paths.emplace_back("player.lua");
    paths.emplace_back("ai/enemy.lua");

    const auto &stored =
        registry.getComponents<RType::ScriptComponent>()[entity];
    REQUIRE(stored.has_value());
    CHECK(stored->scripts ==
        std::vector<std::filesystem::path> {"player.lua", "ai/enemy.lua"});
    CHECK(stored->started == 0);
}
