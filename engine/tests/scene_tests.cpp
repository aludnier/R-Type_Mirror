#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>
#include <vector>

#include "components/Controllable.h"
#include "components/Health.h"
#include "components/Hitbox.h"
#include "components/Name.h"
#include "components/Sprite.h"
#include "components/Transform2D.h"
#include "ecs/Registry.h"
#include "library/RessourceManager.h"
#include "reflection/Reflection.h"
#include "scene/Scene.h"
#include "scripting/ScriptComponent.h"

namespace {
void registerSceneComponents(RType::Registry &registry)
{
    registry.registerComponent<RType::Name>();
    registry.registerComponent<RType::Transform2D>();
    registry.registerComponent<RType::Health>();
    registry.registerComponent<RType::Sprite>();
    registry.registerComponent<RType::ScriptComponent>();
    registry.registerComponent<RType::Hitbox>();
    registry.registerComponent<RType::Controllable>();
}

struct SceneFixture {
    SceneFixture()
    {
        registerSceneComponents(source);
        registerSceneComponents(target);
        RType::reflectCoreComponents(reflection);
        playerTexture = assets.addAsset("player", "assets/player.png");
    }

    RType::Registry source;
    RType::Registry target;
    RType::Reflection reflection;
    ResourceManager assets;
    std::uint32_t playerTexture = 0;
};

RType::Entity spawnPlayer(SceneFixture &scene)
{
    auto player = scene.source.spawnEntity();
    scene.source.addComponent(player, RType::Name {"Player"});
    scene.source.addComponent(player,
        RType::Transform2D {
            RType::Vector2 {120.f, 40.f}, RType::Vector2 {2.f, 3.f}, 0.5f});
    scene.source.addComponent(player, RType::Health {2, 5});
    RType::Sprite sprite(scene.playerTexture, 1, 2, 32, 16, 4);
    sprite.originX = 0.25f;
    sprite.r = 10;
    sprite.a = 128;
    scene.source.addComponent(player, RType::Sprite(sprite));
    scene.source.addComponent(player,
        RType::ScriptComponent {
            .scripts = {"player.lua", "ai/shoot.lua"}, .started = 2});
    scene.source.addComponent(player, RType::Hitbox {24.f, 12.f});
    scene.source.addComponent(player, RType::Controllable {});
    return player;
}
} // namespace

TEST_CASE("Scene round trip keeps every reflected component", "[scene]")
{
    SceneFixture scene;
    auto player = spawnPlayer(scene);
    scene.source.spawnEntity();

    auto spawned = RType::Scene::fromJson(scene.target,
        scene.reflection,
        RType::Scene::toJson(scene.source, scene.reflection, &scene.assets),
        &scene.assets);

    REQUIRE(spawned.size() == 2);
    auto loaded = spawned[0];
    CHECK(scene.target.getComponents<RType::Name>()[loaded]->value == "Player");
    CHECK(scene.target.getComponents<RType::Transform2D>()[loaded] ==
        scene.source.getComponents<RType::Transform2D>()[player]);
    CHECK(scene.target.getComponents<RType::Health>()[loaded] ==
        scene.source.getComponents<RType::Health>()[player]);
    CHECK(scene.target.getComponents<RType::Sprite>()[loaded] ==
        scene.source.getComponents<RType::Sprite>()[player]);
    const auto &script =
        scene.target.getComponents<RType::ScriptComponent>()[loaded];
    REQUIRE(script.has_value());
    CHECK(script->scripts ==
        std::vector<std::filesystem::path> {"player.lua", "ai/shoot.lua"});
    CHECK(script->started == 0);
    CHECK(scene.target.getComponents<RType::Hitbox>()[loaded] ==
        scene.source.getComponents<RType::Hitbox>()[player]);
    CHECK(scene.target.getComponents<RType::Controllable>()[loaded]);
    CHECK_FALSE(scene.target.getComponents<RType::Name>()[spawned[1]]);
}

TEST_CASE("Scene uses the format of the plan", "[scene]")
{
    SceneFixture scene;
    spawnPlayer(scene);

    auto json =
        RType::Scene::toJson(scene.source, scene.reflection, &scene.assets);

    REQUIRE(json["entities"].size() == 1);
    const auto &player = json["entities"][0];
    CHECK(player["name"] == "Player");
    CHECK_FALSE(player["components"].contains("Name"));
    CHECK(player["components"]["Transform2D"]["position"] ==
        nlohmann::json::array({120.f, 40.f}));
    CHECK(player["components"]["Sprite"]["assetId"] == "player");
    CHECK(player["components"]["Script"]["scripts"] ==
        nlohmann::json::array({"player.lua", "ai/shoot.lua"}));
    CHECK_FALSE(player["components"]["Script"].contains("started"));
    CHECK_FALSE(player.contains("id"));
}

TEST_CASE("Scene resolves asset names on load", "[scene]")
{
    SceneFixture scene;
    ResourceManager otherRun;
    otherRun.addAsset("enemy", "assets/enemy.png");
    const auto player = otherRun.addAsset("player", "assets/player.png");
    spawnPlayer(scene);

    auto text = RType::Scene::saveToString(
        scene.source, scene.reflection, &scene.assets);
    auto spawned = RType::Scene::loadFromString(
        scene.target, scene.reflection, text, &otherRun);

    REQUIRE(spawned.size() == 1);
    CHECK(scene.target.getComponents<RType::Sprite>()[spawned[0]]->assetId ==
        player);
}

TEST_CASE("Scene keeps defaults for unknown assets", "[scene]")
{
    SceneFixture scene;
    spawnPlayer(scene);
    auto text = RType::Scene::saveToString(
        scene.source, scene.reflection, &scene.assets);

    ResourceManager empty;
    auto withoutAsset = RType::Scene::loadFromString(
        scene.target, scene.reflection, text, &empty);
    auto withoutManager =
        RType::Scene::loadFromString(scene.target, scene.reflection, text);

    for (auto entity : {withoutAsset[0], withoutManager[0]}) {
        const auto &sprite =
            scene.target.getComponents<RType::Sprite>()[entity];
        REQUIRE(sprite.has_value());
        CHECK(sprite->assetId == 0);
        CHECK(sprite->zIndex == 4);
    }
}

TEST_CASE("Scene skips unknown components and fields", "[scene]")
{
    SceneFixture scene;

    auto spawned = RType::Scene::loadFromString(scene.target,
        scene.reflection,
        R"({"entities": [{"components": {
            "Missing": {"x": 1},
            "Health": {"current": 3, "unknown": 1, "max": "bad"}}}, 42]})");

    REQUIRE(spawned.size() == 1);
    const auto &health =
        scene.target.getComponents<RType::Health>()[spawned[0]];
    REQUIRE(health.has_value());
    CHECK(health->current == 3);
    CHECK(health->max == RType::Health {}.max);
}

TEST_CASE("Scene keeps defaults for missing fields", "[scene]")
{
    SceneFixture scene;

    auto spawned = RType::Scene::loadFromString(scene.target,
        scene.reflection,
        R"({"entities": [{"components": {"Transform2D": {"rotation": 1}}}]})");

    REQUIRE(spawned.size() == 1);
    const auto &transform =
        scene.target.getComponents<RType::Transform2D>()[spawned[0]];
    REQUIRE(transform.has_value());
    CHECK(transform->rotation == 1.f);
    CHECK(transform->position == RType::Vector2 {0.f, 0.f});
    CHECK(transform->scale == RType::Vector2 {1.f, 1.f});
}

TEST_CASE("Scene registers missing storages", "[scene]")
{
    RType::Registry registry;
    registry.registerComponent<RType::Health>();
    RType::Reflection reflection;
    RType::reflectCoreComponents(reflection);

    auto spawned = RType::Scene::loadFromString(registry,
        reflection,
        R"({"entities": [{"name": "A", "components": {
            "Sprite": {"zIndex": 1}, "Health": {"current": 1}}}]})");

    REQUIRE(spawned.size() == 1);
    CHECK(registry.getComponents<RType::Health>()[spawned[0]].has_value());
    CHECK(registry.getComponents<RType::Sprite>()[spawned[0]]->zIndex == 1);
    CHECK(registry.getComponents<RType::Name>()[spawned[0]]->value == "A");
}

TEST_CASE("Scene rejects invalid JSON", "[scene]")
{
    SceneFixture scene;

    CHECK_THROWS(RType::Scene::loadFromString(
        scene.target, scene.reflection, "{not json"));
    CHECK(RType::Scene::loadFromString(scene.target, scene.reflection, "[]")
            .empty());
}

TEST_CASE("Scene saves to and loads from a file", "[scene]")
{
    SceneFixture scene;
    spawnPlayer(scene);
    auto file =
        std::filesystem::temp_directory_path() / "rtype_scene_test.json";

    RType::Scene::saveToFile(
        scene.source, scene.reflection, file, &scene.assets);
    auto spawned = RType::Scene::loadFromFile(
        scene.target, scene.reflection, file, &scene.assets);
    std::filesystem::remove(file);

    REQUIRE(spawned.size() == 1);
    CHECK(scene.target.getComponents<RType::Name>()[spawned[0]]->value ==
        "Player");
    CHECK_THROWS(RType::Scene::loadFromFile(
        scene.target, scene.reflection, file, &scene.assets));
}

TEST_CASE("Scene clear removes every entity", "[scene]")
{
    SceneFixture scene;
    spawnPlayer(scene);
    spawnPlayer(scene);

    RType::Scene::clear(scene.source);

    CHECK(scene.source.entities().empty());
    for (const auto &name : scene.source.getComponents<RType::Name>())
        CHECK_FALSE(name.has_value());
}
