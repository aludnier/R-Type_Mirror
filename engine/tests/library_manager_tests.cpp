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

#include <SFML/Graphics.hpp>
#include "TempConfigFile.hpp"
#include "TestActions.h"
#include "components/Sprite.h"
#include "components/Transform2D.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"
#include "library/AssetManager.h"
#include "library/ClientAssetManager.h"
#include "library/InputManager.h"
#include "systems/RenderSystem.h"

#include "catch2/internal/catch_getenv.hpp"

namespace {
constexpr RType::ActionMask bit(RType::ActionId action)
{
    return RType::ActionMask {1} << action;
}

void defineActions(InputManager &manager)
{
    manager.defineAction("MoveUp", Action::MoveUp);
    manager.defineAction("MoveDown", Action::MoveDown);
    manager.defineAction("MoveLeft", Action::MoveLeft);
    manager.defineAction("MoveRight", Action::MoveRight);
    manager.defineAction("Shoot", Action::Shoot);
}
} // namespace

TEST_CASE("InputManager starts with no active action and an empty mask",
    "[input][manager]")
{
    InputManager manager;

    for (std::size_t i = 0; i < static_cast<std::size_t>(Action::Count); ++i) {
        REQUIRE_FALSE(manager.isActionDown(static_cast<RType::ActionId>(i)));
    }
    REQUIRE(manager.actionMask() == 0);
}

TEST_CASE("InputManager ignores keys that are not bound to any action",
    "[input][manager]")
{
    InputManager manager;

    manager.bind(Action::Shoot, 30);
    manager.setKeyState(77, true);

    REQUIRE_FALSE(manager.isActionDown(Action::Shoot));
    REQUIRE(manager.actionMask() == 0);
}

TEST_CASE("InputManager activates an action when any of its keys is down",
    "[input][manager]")
{
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

TEST_CASE(
    "InputManager treats repeated key states as idempotent", "[input][manager]")
{
    InputManager manager;

    manager.bind(Action::MoveRight, 5);

    manager.setKeyState(5, true);
    manager.setKeyState(5, true);
    manager.setKeyState(5, false);
    REQUIRE_FALSE(manager.isActionDown(Action::MoveRight));

    manager.setKeyState(5, false);
    REQUIRE_FALSE(manager.isActionDown(Action::MoveRight));
}

TEST_CASE(
    "InputManager lets one key trigger several actions", "[input][manager]")
{
    InputManager manager;

    manager.bind(Action::Shoot, 9);
    manager.bind(Action::MoveDown, 9);
    manager.setKeyState(9, true);

    REQUIRE(manager.isActionDown(Action::Shoot));
    REQUIRE(manager.isActionDown(Action::MoveDown));
    REQUIRE(
        manager.actionMask() == (bit(Action::Shoot) | bit(Action::MoveDown)));
}

TEST_CASE(
    "InputManager actionMask combines every active action", "[input][manager]")
{
    InputManager manager;

    manager.bind(Action::MoveUp, 1);
    manager.bind(Action::MoveLeft, 2);
    manager.bind(Action::Shoot, 3);

    manager.setKeyState(1, true);
    manager.setKeyState(3, true);
    REQUIRE(manager.actionMask() == (bit(Action::MoveUp) | bit(Action::Shoot)));

    manager.setKeyState(2, true);
    REQUIRE(manager.actionMask() ==
        (bit(Action::MoveUp) | bit(Action::MoveLeft) | bit(Action::Shoot)));

    manager.setKeyState(1, false);
    manager.setKeyState(2, false);
    manager.setKeyState(3, false);
    REQUIRE(manager.actionMask() == 0);
}

TEST_CASE("InputManager clearBindings removes every key of one action only",
    "[input][manager]")
{
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

TEST_CASE("InputManager supports remapping an action to a new key",
    "[input][manager]")
{
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

TEST_CASE(
    "InputManager uses the new bindings for keys that are already pressed",
    "[input][manager]")
{
    InputManager manager;

    manager.setKeyState(15, true);
    REQUIRE_FALSE(manager.isActionDown(Action::MoveDown));

    manager.bind(Action::MoveDown, 15);
    REQUIRE(manager.isActionDown(Action::MoveDown));
}

TEST_CASE("InputManager loads action bindings from a config file",
    "[input][manager][config]")
{
    TempConfigFile config(R"({
        "bindings": {
            "MoveUp": [10, 11],
            "Shoot": [32]
        }
    })");
    InputManager manager;

    defineActions(manager);
    manager.loadConfigFile(config.path());
    manager.setKeyState(11, true);
    manager.setKeyState(32, true);
    REQUIRE(manager.isActionDown(Action::MoveUp));
    REQUIRE(manager.isActionDown(Action::Shoot));
    REQUIRE_FALSE(manager.isActionDown(Action::MoveDown));
}

TEST_CASE("InputManager accepts named device inputs in a config file",
    "[input][manager][config]")
{
    TempConfigFile config(R"({
        "bindings": {
            "MoveUp": ["KEYBOARD_A", "JOYPAD_UP"],
            "Shoot": ["MOUSE_LEFT"]
        }
    })");
    InputManager manager;

    defineActions(manager);
    manager.loadConfigFile(config.path());

    manager.setInputState("KEYBOARD_A", true);
    REQUIRE(manager.isActionDown(Action::MoveUp));
    manager.setInputState("KEYBOARD_A", false);
    REQUIRE_FALSE(manager.isActionDown(Action::MoveUp));

    manager.setInputState("JOYPAD_UP", true);
    REQUIRE(manager.isActionDown(Action::MoveUp));
    manager.setInputState("MOUSE_LEFT", true);
    REQUIRE(manager.isActionDown(Action::Shoot));
}

TEST_CASE("InputManager config replaces listed bindings and preserves others",
    "[input][manager][config]")
{
    TempConfigFile config(R"({
        "bindings": {
            "MoveUp": [3]
        }
    })");
    InputManager manager;

    defineActions(manager);
    manager.bind(Action::MoveUp, 1);
    manager.bind(Action::Shoot, 2);

    manager.loadConfigFile(config.path());

    manager.setKeyState(1, true);
    manager.setKeyState(2, true);
    REQUIRE_FALSE(manager.isActionDown(Action::MoveUp));
    REQUIRE(manager.isActionDown(Action::Shoot));
    manager.setKeyState(3, true);
    REQUIRE(manager.isActionDown(Action::MoveUp));
}

TEST_CASE("InputManager rejects unknown actions without partial changes",
    "[input][manager][config]")
{
    TempConfigFile config(R"({
        "bindings": {
            "MoveUp": [3],
            "Unknown": [4]
        }
    })");
    InputManager manager;

    defineActions(manager);
    manager.bind(Action::MoveUp, 1);

    REQUIRE_THROWS_AS(
        manager.loadConfigFile(config.path()), std::invalid_argument);
    manager.setKeyState(1, true);
    REQUIRE(manager.isActionDown(Action::MoveUp));
    manager.setKeyState(1, false);
    manager.setKeyState(3, true);
    REQUIRE_FALSE(manager.isActionDown(Action::MoveUp));
}

// ---------------------------------------------------------------------------
// AssetManager
// ---------------------------------------------------------------------------

TEST_CASE("AssetManager gives a different non-zero id to each asset",
    "[resource][manager]")
{
    AssetManager manager;

    const auto a =
        manager.addAsset("a", "assets/a.png", AssetManager::AssetType::Texture);
    const auto b =
        manager.addAsset("b", "assets/b.png", AssetManager::AssetType::Texture);
    const auto c =
        manager.addAsset("c", "assets/c.wav", AssetManager::AssetType::Sound);

    REQUIRE(a != 0);
    REQUIRE(b != 0);
    REQUIRE(c != 0);
    REQUIRE(std::set<std::uint32_t> {a, b, c}.size() == 3);
}

TEST_CASE("AssetManager requires an explicit asset type", "[resource][manager]")
{
    AssetManager manager;

    REQUIRE_THROWS_AS(
        manager.addAsset("bg", "assets/bg.png", ""), std::invalid_argument);
}

TEST_CASE("AssetManager stores every asset type", "[resource][manager]")
{
    AssetManager manager;

    manager.addAsset("t", "t.png", AssetManager::AssetType::Texture);
    manager.addAsset("s", "s.wav", AssetManager::AssetType::Sound);
    manager.addAsset("m", "m.ogg", AssetManager::AssetType::Music);
    manager.addAsset("f", "f.ttf", AssetManager::AssetType::Font);

    REQUIRE(manager.getAsset("t").type == AssetManager::AssetType::Texture);
    REQUIRE(manager.getAsset("s").type == AssetManager::AssetType::Sound);
    REQUIRE(manager.getAsset("m").type == AssetManager::AssetType::Music);
    REQUIRE(manager.getAsset("f").type == AssetManager::AssetType::Font);
}

TEST_CASE("AssetManager constructor loads assets from a config file",
    "[resource][manager][config]")
{
    TempConfigFile config(R"({
        "assets": {
            "background": {"path": "assets/background.png", "type": "Texture"},
            "laser": {"path": "assets/laser.wav", "type": "Sound"},
            "theme": {"path": "assets/theme.ogg", "type": "Music"},
            "font": {"path": "assets/font.ttf", "type": "Font"}
        }
    })");

    AssetManager manager(config.path());

    REQUIRE(manager.hasAsset("background"));
    REQUIRE(manager.getAsset("background").path == "assets/background.png");
    REQUIRE(manager.getAsset("background").type ==
        AssetManager::AssetType::Texture);
    REQUIRE(manager.getAsset("laser").type == AssetManager::AssetType::Sound);
    REQUIRE(manager.getAsset("theme").type == AssetManager::AssetType::Music);
    REQUIRE(manager.getAsset("font").type == AssetManager::AssetType::Font);
}

TEST_CASE("AssetManager loadConfigFile preserves existing asset storage",
    "[resource][manager][config]")
{
    TempConfigFile config(R"({
        "assets": {
            "ship": {"path": "assets/ship_updated.png", "type": "Texture"},
            "laser": {"path": "assets/laser.wav", "type": "Sound"}
        }
    })");
    AssetManager manager;
    const auto shipId = manager.addAsset(
        "ship", "assets/ship.png", AssetManager::AssetType::Texture);

    manager.loadConfigFile(config.path());

    REQUIRE(manager.getAsset("ship").id == shipId);
    REQUIRE(manager.getAsset("ship").path == "assets/ship_updated.png");
    REQUIRE(
        manager.getAssetById(shipId)->get().path == "assets/ship_updated.png");
    REQUIRE(manager.getAsset("laser").type == AssetManager::AssetType::Sound);
}

TEST_CASE("AssetManager rejects invalid config without partial changes",
    "[resource][manager][config]")
{
    TempConfigFile config(R"({
        "assets": {
            "a_valid": {"path": "assets/valid.png", "type": "Valid"},
            "z_invalid": {"path": "", "type": "Unknown"}
        }
    })");
    AssetManager manager;

    REQUIRE_THROWS_AS(
        manager.loadConfigFile(config.path()), std::invalid_argument);
    REQUIRE_FALSE(manager.hasAsset("a_valid"));
    REQUIRE_FALSE(manager.hasAsset("z_invalid"));
}

TEST_CASE("AssetManager reports unknown names without creating them",
    "[resource][manager]")
{
    AssetManager manager;

    REQUIRE_FALSE(manager.hasAsset("ghost"));
    REQUIRE_THROWS_AS(manager.getAsset("ghost"), std::out_of_range);

    // A failed lookup must not silently insert an entry.
    REQUIRE_FALSE(manager.hasAsset("ghost"));
}

TEST_CASE("AssetManager never resolves id 0", "[resource][manager]")
{
    AssetManager manager;

    manager.addAsset(
        "ship", "assets/ship.png", AssetManager::AssetType::Texture);

    REQUIRE_FALSE(manager.getAssetById(0).has_value());
}

TEST_CASE("AssetManager removing an unknown asset does nothing",
    "[resource][manager]")
{
    AssetManager manager;

    const auto id = manager.addAsset(
        "ship", "assets/ship.png", AssetManager::AssetType::Texture);

    REQUIRE_THROWS_AS(manager.removeAsset("ghost"), std::out_of_range);
    REQUIRE(manager.hasAsset("ship"));
    REQUIRE(manager.getAssetById(id).has_value());
}

namespace {
struct FakeAsset {
    static constexpr std::string_view JSON_TYPE = "Fake";
    inline static int loads = 0;
    inline static int destroyed = 0;
    std::string path;

    ~FakeAsset()
    {
        ++destroyed;
    }

    static FakeAsset load(const std::filesystem::path &path)
    {
        ++loads;
        return FakeAsset {path.string()};
    }
};

struct OtherFakeAsset {
    static constexpr std::string_view JSON_TYPE = "OtherFake";

    static OtherFakeAsset load(const std::filesystem::path &)
    {
        return {};
    }
};

struct ConflictingFakeAsset {
    static constexpr std::string_view JSON_TYPE = "Fake";

    static ConflictingFakeAsset load(const std::filesystem::path &)
    {
        return {};
    }
};

struct FailingFakeAsset {
    static constexpr std::string_view JSON_TYPE = "FailingFake";

    static FailingFakeAsset load(const std::filesystem::path &path)
    {
        throw std::runtime_error(
            "fake loader failed for '" + path.string() + "'");
    }
};

struct EmptyTypeAsset {
    static constexpr std::string_view JSON_TYPE {};

    static EmptyTypeAsset load(const std::filesystem::path &)
    {
        return {};
    }
};
} // namespace

static_assert(FakeAsset::JSON_TYPE == std::string_view {"Fake"});
static_assert(AssetTraits<FakeAsset>::jsonType == std::string_view {"Fake"});

TEST_CASE("ClientAssetManager dispatches, caches, and invalidates typed assets",
    "[resource][client]")
{
    FakeAsset::loads = 0;
    FakeAsset::destroyed = 0;
    ClientAssetManager manager;
    manager.metadata().addAsset("one", "one.fake", "Fake");
    manager.registerType<FakeAsset>();

    REQUIRE(manager.load<FakeAsset>("one").path == "one.fake");
    REQUIRE(manager.load<FakeAsset>("one").path == "one.fake");
    REQUIRE(FakeAsset::loads == 1);

    manager.metadata().addAsset("one", "two.fake", "Fake");
    REQUIRE(manager.load<FakeAsset>("one").path == "two.fake");
    REQUIRE(FakeAsset::loads == 2);

    const auto destroyedBeforeUnload = FakeAsset::destroyed;
    manager.unload("one");
    REQUIRE(FakeAsset::destroyed > destroyedBeforeUnload);
    REQUIRE(manager.load<FakeAsset>("one").path == "two.fake");
    REQUIRE(FakeAsset::loads == 3);
    manager.clear();
    REQUIRE(manager.load<FakeAsset>("one").path == "two.fake");
    REQUIRE(FakeAsset::loads == 4);
}

TEST_CASE("ClientAssetManager rejects invalid registrations and dispatch",
    "[resource][client]")
{
    ClientAssetManager manager;
    manager.metadata().addAsset("one", "one.fake", "Fake");
    manager.registerType<FakeAsset>();

    REQUIRE_THROWS_AS(manager.registerType<FakeAsset>(), std::invalid_argument);
    REQUIRE_NOTHROW(manager.registerType<OtherFakeAsset>());
    REQUIRE_THROWS_AS(
        manager.registerType<ConflictingFakeAsset>(), std::invalid_argument);
    REQUIRE_THROWS_AS(
        manager.registerType<EmptyTypeAsset>(), std::invalid_argument);
    REQUIRE_THROWS_AS(
        manager.load<OtherFakeAsset>("one"), std::invalid_argument);

    manager.metadata().addAsset("unknown", "unknown.asset", "Missing");
    REQUIRE_THROWS_AS(manager.preload("unknown"), std::invalid_argument);
    REQUIRE_THROWS_AS(manager.load<FakeAsset>("missing"), std::out_of_range);
}

TEST_CASE("AssetManager rejects missing config asset types",
    "[resource][manager][config]")
{
    TempConfigFile config(R"({
        "assets": {
            "missing_type": {"path": "asset.fake"}
        }
    })");
    AssetManager manager;

    REQUIRE_THROWS_AS(
        manager.loadConfigFile(config.path()), std::invalid_argument);
    REQUIRE_FALSE(manager.hasAsset("missing_type"));
}

TEST_CASE("ClientAssetManager reports loader failures and unknown unloads",
    "[resource][client]")
{
    ClientAssetManager manager;
    manager.metadata().addAsset("failing", "asset.fake", "FailingFake");
    manager.registerType<FailingFakeAsset>();

    REQUIRE_THROWS_AS(manager.preload("failing"), std::runtime_error);
    REQUIRE_THROWS_AS(manager.unload("missing"), std::out_of_range);
}

TEST_CASE(
    "AssetManager removing one asset keeps the others", "[resource][manager]")
{
    AssetManager manager;

    const auto keepId = manager.addAsset(
        "keep", "assets/keep.png", AssetManager::AssetType::Texture);
    const auto dropId = manager.addAsset(
        "drop", "assets/drop.png", AssetManager::AssetType::Texture);

    manager.removeAsset("drop");

    REQUIRE_FALSE(manager.hasAsset("drop"));
    REQUIRE_FALSE(manager.getAssetById(dropId).has_value());
    REQUIRE(manager.hasAsset("keep"));
    REQUIRE(manager.getAssetById(keepId).has_value());
    REQUIRE(manager.getAssetById(keepId)->get().path == "assets/keep.png");
}

TEST_CASE("AssetManager does not reuse the id of a removed asset",
    "[resource][manager]")
{
    AssetManager manager;

    const auto oldId = manager.addAsset(
        "cursor", "assets/cursor.png", AssetManager::AssetType::Texture);
    manager.removeAsset("cursor");
    const auto newId = manager.addAsset(
        "cursor", "assets/cursor2.png", AssetManager::AssetType::Texture);

    REQUIRE(newId != oldId);
    REQUIRE_FALSE(manager.getAssetById(oldId).has_value());
    REQUIRE(manager.getAssetById(newId).has_value());
    REQUIRE(manager.getAssetById(newId)->get().path == "assets/cursor2.png");
}

TEST_CASE("AssetManager keeps lookups by name and by id consistent after an "
          "update",
    "[resource][manager]")
{
    AssetManager manager;

    const auto id = manager.addAsset(
        "music", "assets/old.ogg", AssetManager::AssetType::Music);
    manager.addAsset("music", "assets/new.ogg", AssetManager::AssetType::Music);

    REQUIRE(manager.getAsset("music").path == "assets/new.ogg");
    REQUIRE(manager.getAssetById(id)->get().path == "assets/new.ogg");
    REQUIRE(&manager.getAssetById(id)->get() == &manager.getAsset("music"));
}

// ---------------------------------------------------------------------------
// RenderSystem & Sprite
// ---------------------------------------------------------------------------

TEST_CASE("Sprite component initializes with defaults and custom parameters",
    "[render][sprite]")
{
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

TEST_CASE("RenderSystem manages texture cache and renders entities safely",
    "[render][system]")
{

    if (Catch::Detail::getEnv("DISPLAY") == nullptr) {
        SUCCEED("Skipping render system test in headless environment.");
        return;
    }
    RType::Registry registry;
    registry.registerComponent<RType::Transform2D>();
    registry.registerComponent<RType::Sprite>();

    AssetManager resources;
    const auto textureId = resources.addAsset("non_existing_test_sprite",
        "assets/does_not_exist.png",
        AssetManager::AssetType::Texture);

    auto entity1 = registry.spawnEntity();
    registry.addComponent(
        entity1, RType::Transform2D({100.0f, 200.0f}, {1.0f, 1.0f}, 0.0f));
    registry.addComponent(entity1, RType::Sprite(textureId, 1));

    auto entity2 = registry.spawnEntity();
    registry.addComponent(
        entity2, RType::Transform2D({300.0f, 400.0f}, {2.0f, 2.0f}, 1.57f));
    registry.addComponent(entity2, RType::Sprite(0, 2));

    RType::RenderSystem renderSystem;
    REQUIRE_FALSE(
        renderSystem.getTexture("assets/does_not_exist.png").has_value());

    sf::RenderTexture renderTexture(sf::Vector2u(800, 600));
    // RenderSystem rendering with fallback for missing assets
    REQUIRE_NOTHROW(renderSystem.render(registry, resources, renderTexture));

    renderSystem.clearTextureCache();
}

TEST_CASE("addRenderSystem registers rendering system into ECS registry",
    "[render][system]")
{

    if (Catch::Detail::getEnv("DISPLAY") == nullptr) {
        SUCCEED("Skipping render system registration test in headless "
                "environment.");
        return;
    }
    RType::Registry registry;
    registry.registerComponent<RType::Transform2D>();
    registry.registerComponent<RType::Sprite>();

    AssetManager resources;
    RType::RenderSystem renderSystem;
    sf::RenderTexture renderTexture(sf::Vector2u(800, 600));

    RType::addRenderSystem(registry, resources, renderTexture, renderSystem);
    REQUIRE_NOTHROW(registry.runSystems());
}

TEST_CASE("InputManager loads bindings for actions defined by the game",
    "[input][manager]")
{
    constexpr RType::ActionId jump = 20;
    TempConfigFile config(R"({"bindings": {"Jump": [42]}})");

    InputManager manager;
    manager.defineAction("Jump", jump);
    manager.loadConfigFile(config.path());
    manager.setKeyState(42, true);

    REQUIRE(manager.isActionDown(jump));
    REQUIRE(manager.actionMask() == bit(jump));
}

TEST_CASE(
    "InputManager rejects actions that do not fit the mask", "[input][manager]")
{
    InputManager manager;

    REQUIRE_THROWS_AS(manager.defineAction("TooBig", 32), std::out_of_range);
    REQUIRE_THROWS_AS(manager.bind(32, 42), std::out_of_range);
}
