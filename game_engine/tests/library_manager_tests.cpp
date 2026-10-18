/*
** EPITECH PROJECT, 2026
** RType
** File description:
** library manager tests (extended)
*/

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <set>
#include <stdexcept>

#include "library/InputManager.h"
#include "library/RessourceManager.h"
#include "components/Sprite.h"
#include "components/Transform2D.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"
#include "systems/RenderSystem.h"
#include <SFML/Graphics.hpp>

namespace {
constexpr std::uint8_t bit(Action action) {
    return static_cast<std::uint8_t>(1u << static_cast<std::size_t>(action));
}
}

TEST_CASE("InputManager starts with no active action and an empty mask", "[input][manager]") {
    InputManager manager;

    for (std::size_t i = 0; i < static_cast<std::size_t>(Action::Count); ++i) {
        REQUIRE_FALSE(manager.isActionDown(static_cast<Action>(i)));
    }
    REQUIRE(manager.actionMask() == 0);
}

TEST_CASE("InputManager ignores keys that are not bound to any action", "[input][manager]") {
    InputManager manager;

    manager.bind(Action::Shoot, 30);
    manager.setKeyState(77, true);

    REQUIRE_FALSE(manager.isActionDown(Action::Shoot));
    REQUIRE(manager.actionMask() == 0);
}

TEST_CASE("InputManager activates an action when any of its keys is down", "[input][manager]") {
    InputManager manager;

    manager.bind(Action::MoveUp, 1);
    manager.bind(Action::MoveUp, 2);

    manager.setKeyState(1, true);
    manager.setKeyState(2, true);
    REQUIRE(manager.isActionDown(Action::MoveUp));

    manager.setKeyState(1, false);
    REQUIRE(manager.isActionDown(Action::MoveUp));

    manager.setKeyState(2, false);
    REQUIRE_FALSE(manager.isActionDown(Action::MoveUp));
}

TEST_CASE("InputManager treats repeated key states as idempotent", "[input][manager]") {
    InputManager manager;

    manager.bind(Action::MoveRight, 5);

    manager.setKeyState(5, true);
    manager.setKeyState(5, true);
    manager.setKeyState(5, false);
    REQUIRE_FALSE(manager.isActionDown(Action::MoveRight));

    manager.setKeyState(5, false);
    REQUIRE_FALSE(manager.isActionDown(Action::MoveRight));
}

TEST_CASE("InputManager lets one key trigger several actions", "[input][manager]") {
    InputManager manager;

    manager.bind(Action::Shoot, 9);
    manager.bind(Action::MoveDown, 9);
    manager.setKeyState(9, true);

    REQUIRE(manager.isActionDown(Action::Shoot));
    REQUIRE(manager.isActionDown(Action::MoveDown));
    REQUIRE(manager.actionMask() == (bit(Action::Shoot) | bit(Action::MoveDown)));
}

TEST_CASE("InputManager actionMask combines every active action", "[input][manager]") {
    InputManager manager;

    manager.bind(Action::MoveUp, 1);
    manager.bind(Action::MoveLeft, 2);
    manager.bind(Action::Shoot, 3);

    manager.setKeyState(1, true);
    manager.setKeyState(3, true);
    REQUIRE(manager.actionMask() == (bit(Action::MoveUp) | bit(Action::Shoot)));

    manager.setKeyState(2, true);
    REQUIRE(manager.actionMask()
            == (bit(Action::MoveUp) | bit(Action::MoveLeft) | bit(Action::Shoot)));

    manager.setKeyState(1, false);
    manager.setKeyState(2, false);
    manager.setKeyState(3, false);
    REQUIRE(manager.actionMask() == 0);
}

TEST_CASE("InputManager clearBindings removes every key of one action only", "[input][manager]") {
    InputManager manager;

    manager.bind(Action::MoveUp, 1);
    manager.bind(Action::MoveUp, 2);
    manager.bind(Action::Shoot, 3);
    manager.setKeyState(1, true);
    manager.setKeyState(2, true);
    manager.setKeyState(3, true);

    manager.clearBindings(Action::MoveUp);

    REQUIRE_FALSE(manager.isActionDown(Action::MoveUp));
    REQUIRE(manager.isActionDown(Action::Shoot));
}

TEST_CASE("InputManager supports remapping an action to a new key", "[input][manager]") {
    InputManager manager;

    manager.bind(Action::Shoot, 32);
    manager.setKeyState(32, true);
    REQUIRE(manager.isActionDown(Action::Shoot));

    manager.clearBindings(Action::Shoot);
    manager.bind(Action::Shoot, 90);

    REQUIRE_FALSE(manager.isActionDown(Action::Shoot));

    manager.setKeyState(90, true);
    REQUIRE(manager.isActionDown(Action::Shoot));
}

TEST_CASE("InputManager uses the new bindings for keys that are already pressed", "[input][manager]") {
    InputManager manager;

    manager.setKeyState(15, true);
    REQUIRE_FALSE(manager.isActionDown(Action::MoveDown));

    manager.bind(Action::MoveDown, 15);
    REQUIRE(manager.isActionDown(Action::MoveDown));
}

// ---------------------------------------------------------------------------
// ResourceManager
// ---------------------------------------------------------------------------

TEST_CASE("ResourceManager gives a different non-zero id to each asset", "[resource][manager]") {
    ResourceManager manager;

    const auto a = manager.addAsset("a", "assets/a.png");
    const auto b = manager.addAsset("b", "assets/b.png");
    const auto c = manager.addAsset("c", "assets/c.wav", ResourceManager::AssetType::Sound);

    REQUIRE(a != 0);
    REQUIRE(b != 0);
    REQUIRE(c != 0);
    REQUIRE(std::set<std::uint32_t>{a, b, c}.size() == 3);
}

TEST_CASE("ResourceManager defaults the asset type to Texture", "[resource][manager]") {
    ResourceManager manager;

    manager.addAsset("bg", "assets/bg.png");

    REQUIRE(manager.getAsset("bg").type == ResourceManager::AssetType::Texture);
}

TEST_CASE("ResourceManager stores every asset type", "[resource][manager]") {
    ResourceManager manager;

    manager.addAsset("t", "t.png", ResourceManager::AssetType::Texture);
    manager.addAsset("s", "s.wav", ResourceManager::AssetType::Sound);
    manager.addAsset("m", "m.ogg", ResourceManager::AssetType::Music);
    manager.addAsset("f", "f.ttf", ResourceManager::AssetType::Font);

    REQUIRE(manager.getAsset("t").type == ResourceManager::AssetType::Texture);
    REQUIRE(manager.getAsset("s").type == ResourceManager::AssetType::Sound);
    REQUIRE(manager.getAsset("m").type == ResourceManager::AssetType::Music);
    REQUIRE(manager.getAsset("f").type == ResourceManager::AssetType::Font);
}

TEST_CASE("ResourceManager reports unknown names without creating them", "[resource][manager]") {
    ResourceManager manager;

    REQUIRE_FALSE(manager.hasAsset("ghost"));
    REQUIRE_THROWS_AS(manager.getAsset("ghost"), std::out_of_range);

    // A failed lookup must not silently insert an entry.
    REQUIRE_FALSE(manager.hasAsset("ghost"));
}

TEST_CASE("ResourceManager never resolves id 0", "[resource][manager]") {
    ResourceManager manager;

    manager.addAsset("ship", "assets/ship.png");

    REQUIRE_FALSE(manager.getAssetById(0).has_value());
}

TEST_CASE("ResourceManager removing an unknown asset does nothing", "[resource][manager]") {
    ResourceManager manager;

    const auto id = manager.addAsset("ship", "assets/ship.png");

    REQUIRE_NOTHROW(manager.removeAsset("ghost"));
    REQUIRE(manager.hasAsset("ship"));
    REQUIRE(manager.getAssetById(id).has_value());
}

TEST_CASE("ResourceManager removing one asset keeps the others", "[resource][manager]") {
    ResourceManager manager;

    const auto keepId = manager.addAsset("keep", "assets/keep.png");
    const auto dropId = manager.addAsset("drop", "assets/drop.png");

    manager.removeAsset("drop");

    REQUIRE_FALSE(manager.hasAsset("drop"));
    REQUIRE_FALSE(manager.getAssetById(dropId).has_value());
    REQUIRE(manager.hasAsset("keep"));
    REQUIRE(manager.getAssetById(keepId).has_value());
    REQUIRE(manager.getAssetById(keepId)->get().path == "assets/keep.png");
}

TEST_CASE("ResourceManager does not reuse the id of a removed asset", "[resource][manager]") {
    ResourceManager manager;

    const auto oldId = manager.addAsset("cursor", "assets/cursor.png");
    manager.removeAsset("cursor");
    const auto newId = manager.addAsset("cursor", "assets/cursor2.png");

    REQUIRE(newId != oldId);
    REQUIRE_FALSE(manager.getAssetById(oldId).has_value());
    REQUIRE(manager.getAssetById(newId).has_value());
    REQUIRE(manager.getAssetById(newId)->get().path == "assets/cursor2.png");
}

TEST_CASE("ResourceManager keeps lookups by name and by id consistent after an update", "[resource][manager]") {
    ResourceManager manager;

    const auto id = manager.addAsset("music", "assets/old.ogg", ResourceManager::AssetType::Music);
    manager.addAsset("music", "assets/new.ogg", ResourceManager::AssetType::Music);

    REQUIRE(manager.getAsset("music").path == "assets/new.ogg");
    REQUIRE(manager.getAssetById(id)->get().path == "assets/new.ogg");
    REQUIRE(&manager.getAssetById(id)->get() == &manager.getAsset("music"));
}

// ---------------------------------------------------------------------------
// RenderSystem & Sprite
// ---------------------------------------------------------------------------

TEST_CASE("Sprite component initializes with defaults and custom parameters", "[render][sprite]") {
    RType::Sprite defaultSprite;
    REQUIRE(defaultSprite.assetId == 0);
    REQUIRE(defaultSprite.zIndex == 0);
    REQUIRE(defaultSprite.rectWidth == 0);
    REQUIRE(defaultSprite.rectHeight == 0);

    RType::Sprite customSprite(42, 10, 20, 100, 200, 5);
    REQUIRE(customSprite.assetId == 42);
    REQUIRE(customSprite.rectLeft == 10);
    REQUIRE(customSprite.rectTop == 20);
    REQUIRE(customSprite.rectWidth == 100);
    REQUIRE(customSprite.rectHeight == 200);
    REQUIRE(customSprite.zIndex == 5);
}

TEST_CASE("RenderSystem manages texture cache and renders entities safely", "[render][system]") {
    RType::Registry registry;
    registry.registerComponent<RType::Transform2D>();
    registry.registerComponent<RType::Sprite>();

    ResourceManager resources;
    const auto textureId = resources.addAsset("non_existing_test_sprite", "assets/does_not_exist.png");

    auto entity1 = registry.spawnEntity();
    registry.addComponent(entity1, RType::Transform2D({100.0f, 200.0f}, {1.0f, 1.0f}, 0.0f));
    registry.addComponent(entity1, RType::Sprite(textureId, 1));

    auto entity2 = registry.spawnEntity();
    registry.addComponent(entity2, RType::Transform2D({300.0f, 400.0f}, {2.0f, 2.0f}, 1.57f));
    registry.addComponent(entity2, RType::Sprite(0, 2));

    RType::RenderSystem renderSystem;
    REQUIRE_FALSE(renderSystem.getTexture("assets/does_not_exist.png").has_value());

    sf::RenderTexture renderTexture(sf::Vector2u(800, 600));
    // RenderSystem rendering with fallback for missing assets
    REQUIRE_NOTHROW(renderSystem.render(registry, resources, renderTexture));

    renderSystem.clearTextureCache();
}

TEST_CASE("addRenderSystem registers rendering system into ECS registry", "[render][system]") {
    RType::Registry registry;
    registry.registerComponent<RType::Transform2D>();
    registry.registerComponent<RType::Sprite>();

    ResourceManager resources;
    RType::RenderSystem renderSystem;
    sf::RenderTexture renderTexture(sf::Vector2u(800, 600));

    RType::addRenderSystem(registry, resources, renderTexture, renderSystem);
    REQUIRE_NOTHROW(registry.runSystems());
}
