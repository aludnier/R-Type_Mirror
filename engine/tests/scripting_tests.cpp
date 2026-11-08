#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "TestActions.h"
#include "components/Controllable.h"
#include "components/Health.h"
#include "components/Hitbox.h"
#include "components/Name.h"
#include "components/Sprite.h"
#include "components/Transform2D.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"
#include "scripting/ScriptComponent.h"
#include "scripting/ScriptEngine.h"
#include "scripting/ScriptSystem.h"
#include "scripting/UiBindings.h"
#include "ui/UiManager.h"

namespace {
class TempDirectory {
public:
    TempDirectory()
    {
        static std::size_t counter = 0;
        _path = std::filesystem::temp_directory_path() /
            ("rtype_scripting_test_" + std::to_string(counter++));
        std::filesystem::create_directories(_path);
    }

    ~TempDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(_path, error);
    }

    [[nodiscard]] std::filesystem::path write(
        const std::string &name, const std::string &contents) const
    {
        const auto file = _path / name;
        std::ofstream output(file, std::ios::trunc);
        output << contents;
        return file;
    }

    [[nodiscard]] const std::filesystem::path &path() const
    {
        return _path;
    }

private:
    std::filesystem::path _path;
};

void registerCoreComponents(RType::Registry &registry)
{
    registry.registerComponent<RType::Transform2D>();
    registry.registerComponent<RType::Health>();
    registry.registerComponent<RType::Hitbox>();
    registry.registerComponent<RType::Controllable>();
    registry.registerComponent<RType::Name>();
    registry.registerComponent<RType::Sprite>();
}

struct Score {
    int value = 0;
};
} // namespace

TEST_CASE("Scripts spawn entities and add core components", "[scripting]")
{
    RType::Registry registry;
    registerCoreComponents(registry);
    RType::ScriptEngine scripts(registry);
    RType::addScriptSystem(registry, scripts);

    REQUIRE(scripts.loadString(R"(
        player = world.spawn()
        world.add(player, "Transform2D", Transform2D.new(Vector2.new(10, 20)))
        world.add(player, "Health", Health.new(3))
        world.add(player, "Hitbox")
        world.add(player, "Name", Name.new("Player"))
    )"));

    const auto id = scripts.lua()["player"].get<RType::entity_id_type>();
    const auto &transform = registry.getComponents<RType::Transform2D>()[id];
    REQUIRE(transform.has_value());
    CHECK(transform->position == RType::Vector2 {10.f, 20.f});
    CHECK(registry.getComponents<RType::Health>()[id]->max == 3);
    CHECK(registry.getComponents<RType::Hitbox>()[id].has_value());
    CHECK(registry.getComponents<RType::Name>()[id]->value == "Player");
}

TEST_CASE("Lua scripts can show and update UI documents", "[scripting][ui]")
{
    TempDirectory files;
    const auto uiFile = files.write("menu.json", R"({
        "name": "menu",
        "elements": [
            {"type": "input_text", "id": "player", "rect": [0, 0, 1, 0.1], "value": "Pilot"},
            {"type": "checkbox", "id": "music", "rect": [0, 0, 0.5, 0.1]},
            {"type": "slider", "id": "volume", "rect": [0, 0, 0.5, 0.1], "min": 0, "max": 1, "value": 0.5},
            {"type": "button", "id": "play", "rect": [0, 0, 0.2, 0.1]}
        ]
    })");
    sol::state lua;
    lua.open_libraries(sol::lib::base);
    RType::UI::UiManager ui;
    RType::Scripting::bindUi(lua, ui);

    const std::string script = "ui.load('menu', [=[" + uiFile.string() +
        "]=])\n"
        "ui.show('menu')\n"
        "ui.set_text('menu/player', 'Ace')\n"
        "ui.set_value('menu/music', true)\n"
        "music_enabled = ui.get_value('menu/music')\n"
        "ui.set_value('menu/volume', 0.4)\n"
        "volume_level = ui.get_value('menu/volume')\n"
        "ui.on_click('menu/play', function() clicked = true end)\n"
        "clicked = false\n"
        "ui.hide('menu')\n"
        "was_hidden = not ui.is_visible('menu')\n"
        "ui.show('menu')";
    const auto result = lua.safe_script(script);
    REQUIRE(result.valid());
    CHECK(ui.isVisible("menu"));
    CHECK(ui.getText("menu/player") == "Ace");
    CHECK(lua["music_enabled"].get<bool>());
    CHECK(lua["volume_level"].get<float>() == 0.4f);
    CHECK(lua["was_hidden"].get<bool>());
    CHECK(lua["ui"]["get_text"]("menu/player").get<std::string>() == "Ace");

    ui.click("menu/play");
    CHECK(lua["clicked"].get<bool>());
    REQUIRE(
        lua.safe_script(R"(
        click_events = ui.take_clicked_events()
        click_event_count = #click_events
        click_event_key = click_events[1]
        empty_click_event_count = #ui.take_clicked_events()
    )")
            .valid());
    CHECK(lua["click_event_count"].get<int>() == 1);
    CHECK(lua["click_event_key"].get<std::string>() == "menu/play");
    CHECK(lua["empty_click_event_count"].get<int>() == 0);
}

TEST_CASE("world.get returns a live reference to the component", "[scripting]")
{
    RType::Registry registry;
    registerCoreComponents(registry);
    const auto entity = registry.spawnEntity();
    registry.emplaceComponent<RType::Transform2D>(entity);
    registry.emplaceComponent<RType::Health>(entity, 5);
    RType::ScriptEngine scripts(registry);
    RType::addScriptSystem(registry, scripts);
    scripts.lua()["e"] = static_cast<RType::entity_id_type>(entity);

    REQUIRE(scripts.loadString(R"(
        local t = world.get(e, "Transform2D")
        t.position.x = 42
        t.rotation = 1.5
        world.get(e, "Health"):take_damage(2)
        missing = world.get(e, "Controllable")
        has_health = world.has(e, "Health")
    )"));

    const auto &transform =
        registry.getComponents<RType::Transform2D>()[entity];
    CHECK(transform->position.x == 42.f);
    CHECK(transform->rotation == 1.5f);
    CHECK(registry.getComponents<RType::Health>()[entity]->current == 3);
    CHECK_FALSE(scripts.lua()["missing"].valid());
    CHECK(scripts.lua()["has_health"].get<bool>());
}

TEST_CASE("Script systems run with the registry and receive the delta time",
    "[scripting]")
{
    RType::Registry registry;
    registerCoreComponents(registry);
    RType::ScriptEngine scripts(registry);
    RType::addScriptSystem(registry, scripts);

    REQUIRE(scripts.loadString(R"(
        ship = world.spawn()
        world.add(ship, "Transform2D")
        world.add_system("move_right", function(dt)
            world.each({"Transform2D"}, function(e, t)
                t.position = t.position + Vector2.new(100, 0) * dt
            end)
        end)
    )"));
    REQUIRE(scripts.systemCount() == 1);

    scripts.setDeltaTime(0.5f);
    registry.runSystems();
    registry.runSystems();

    const auto id = scripts.lua()["ship"].get<RType::entity_id_type>();
    CHECK(
        registry.getComponents<RType::Transform2D>()[id]->position.x == 100.f);
}

TEST_CASE(
    "world.each only visits entities having every component", "[scripting]")
{
    RType::Registry registry;
    registerCoreComponents(registry);
    RType::ScriptEngine scripts(registry);
    RType::addScriptSystem(registry, scripts);

    REQUIRE(scripts.loadString(R"(
        local a = world.spawn()
        world.add(a, "Transform2D")
        world.add(a, "Health", Health.new(1))
        local b = world.spawn()
        world.add(b, "Transform2D")
        local c = world.spawn()
        world.add(c, "Health", Health.new(1))
        visited = {}
        world.each({"Transform2D", "Health"}, function(e, t, h)
            visited[#visited + 1] = e
        end)
        expected = a
    )"));

    sol::table visited = scripts.lua()["visited"];
    REQUIRE(visited.size() == 1);
    CHECK(visited.get<int>(1) == scripts.lua()["expected"].get<int>());
}

TEST_CASE("Killed entities lose their components at the end of the frame",
    "[scripting]")
{
    RType::Registry registry;
    registerCoreComponents(registry);
    RType::ScriptEngine scripts(registry);
    RType::addScriptSystem(registry, scripts);

    REQUIRE(scripts.loadString(R"(
        enemy = world.spawn()
        world.add(enemy, "Health", Health.new(1))
        world.data(enemy).kind = "bydo"
        world.add_system("die", function(dt)
            world.each({"Health"}, function(e, h)
                h:take_damage(1)
                if h:is_dead() then world.kill(e) end
            end)
        end)
    )"));

    registry.runSystems();

    const auto id = scripts.lua()["enemy"].get<RType::entity_id_type>();
    CHECK_FALSE(registry.getComponents<RType::Health>()[id].has_value());
    CHECK_FALSE(registry.getComponents<RType::ScriptData>()[id].has_value());
}

TEST_CASE("A kill handler replaces what world.kill does", "[scripting]")
{
    RType::Registry registry;
    registerCoreComponents(registry);
    RType::ScriptEngine scripts(registry);
    RType::addScriptSystem(registry, scripts);
    std::vector<RType::entity_id_type> killed;
    scripts.setKillHandler([&killed](RType::Entity entity) {
        killed.push_back(static_cast<RType::entity_id_type>(entity));
    });

    REQUIRE(scripts.loadString(R"(
        enemy = world.spawn()
        world.add(enemy, "Health", Health.new(1))
        world.kill(enemy)
    )"));
    registry.runSystems();

    const auto id = scripts.lua()["enemy"].get<RType::entity_id_type>();
    REQUIRE(killed == std::vector {id});
    // The handler did not kill it: the entity is still there.
    CHECK(registry.getComponents<RType::Health>()[id].has_value());

    scripts.setKillHandler(nullptr);
    REQUIRE(scripts.loadString("world.kill(enemy)"));
    registry.runSystems();

    CHECK(killed.size() == 1);
    CHECK_FALSE(registry.getComponents<RType::Health>()[id].has_value());
}

TEST_CASE("world.data keeps per-entity Lua fields", "[scripting]")
{
    RType::Registry registry;
    RType::ScriptEngine scripts(registry);
    RType::addScriptSystem(registry, scripts);

    REQUIRE(scripts.loadString(R"(
        local e = world.spawn()
        world.data(e).timer = 2
        world.data(e).timer = world.data(e).timer + 1
        timer = world.data(e).timer
    )"));

    CHECK(scripts.lua()["timer"].get<int>() == 3);
}

TEST_CASE("Controllable exposes the input actions", "[scripting]")
{
    RType::Registry registry;
    registerCoreComponents(registry);
    const auto entity = registry.spawnEntity();
    registry.emplaceComponent<RType::Controllable>(entity);
    RType::ScriptEngine scripts(registry);
    scripts.defineAction("MoveUp", Action::MoveUp);
    scripts.defineAction("Shoot", Action::Shoot);
    RType::addScriptSystem(registry, scripts);
    scripts.lua()["e"] = static_cast<RType::entity_id_type>(entity);

    REQUIRE(scripts.loadString(R"(
        local c = world.get(e, "Controllable")
        c:set_active(Action.Shoot, true)
        c:set_active(Action.MoveUp, true)
        c:set_active(Action.MoveUp, false)
        shooting = c:is_active(Action.Shoot)
    )"));

    CHECK(scripts.lua()["shooting"].get<bool>());
    CHECK(registry.getComponents<RType::Controllable>()[entity]->actionMask ==
        (1U << static_cast<unsigned>(Action::Shoot)));
}

TEST_CASE(
    "Components declared by the host are usable from scripts", "[scripting]")
{
    RType::Registry registry;
    registry.registerComponent<Score>();
    RType::ScriptEngine scripts(registry);
    RType::addScriptSystem(registry, scripts);
    scripts.lua().new_usertype<Score>("Score", "value", &Score::value);
    scripts.bindComponent<Score>("Score");

    REQUIRE(scripts.loadString(R"(
        local e = world.spawn()
        world.add(e, "Score").value = 10
        id = e
    )"));

    const auto id = scripts.lua()["id"].get<RType::entity_id_type>();
    CHECK(registry.getComponents<Score>()[id]->value == 10);
}

TEST_CASE("Script errors are reported without crashing the host", "[scripting]")
{
    RType::Registry registry;
    registerCoreComponents(registry);
    RType::ScriptEngine scripts(registry);
    RType::addScriptSystem(registry, scripts);

    SECTION("syntax error")
    {
        CHECK_FALSE(scripts.loadString("this is not lua", "broken"));
        CHECK_THAT(
            scripts.lastError(), Catch::Matchers::ContainsSubstring("broken"));
    }

    SECTION("unknown component")
    {
        CHECK_FALSE(scripts.loadString("world.add(world.spawn(), 'Nope')"));
        CHECK_THAT(scripts.lastError(),
            Catch::Matchers::ContainsSubstring("unknown component 'Nope'"));
    }

    SECTION("wrong component value")
    {
        CHECK_FALSE(scripts.loadString(
            "world.add(world.spawn(), 'Health', Vector2.new(1, 2))"));
        CHECK_THAT(scripts.lastError(),
            Catch::Matchers::ContainsSubstring("not a Health"));
    }

    SECTION("negative entity id")
    {
        CHECK_FALSE(scripts.loadString("world.kill(-1)"));
        CHECK_THAT(scripts.lastError(),
            Catch::Matchers::ContainsSubstring("invalid entity id"));
    }

    SECTION("failing system is disabled")
    {
        REQUIRE(scripts.loadString(R"(
            calls = 0
            world.add_system("broken", function(dt)
                calls = calls + 1
                error("boom")
            end)
        )"));
        registry.runSystems();
        registry.runSystems();
        CHECK(scripts.lua()["calls"].get<int>() == 1);
        CHECK_THAT(scripts.lastError(),
            Catch::Matchers::ContainsSubstring("system 'broken'"));
    }
}

TEST_CASE("Scripts run in a sandbox", "[scripting]")
{
    RType::Registry registry;
    RType::ScriptEngine scripts(registry);
    RType::addScriptSystem(registry, scripts);

    REQUIRE(scripts.loadString(R"(
        sandboxed = io == nil and os == nil and package == nil
            and debug == nil and dofile == nil and loadfile == nil
            and load == nil and require == nil
    )"));

    CHECK(scripts.lua()["sandboxed"].get<bool>());
}

TEST_CASE("Scripts are loaded from a JSON config file and can be reloaded",
    "[scripting]")
{
    TempDirectory directory;
    std::filesystem::create_directories(directory.path() / "scripts");
    const auto script = directory.write("scripts/game.lua", R"(
        counter = (counter or 0) + 1
        world.add_system("tick", function(dt) end)
    )");
    const auto config = directory.write(
        "game.json", R"({"directory": "scripts", "scripts": ["game.lua"]})");
    RType::Registry registry;
    RType::ScriptEngine scripts(registry);
    RType::addScriptSystem(registry, scripts);

    scripts.loadConfigFile(config);
    REQUIRE(scripts.lua()["counter"].get<int>() == 1);
    REQUIRE(scripts.systemCount() == 1);

    SECTION("reload replays the files and replaces the systems")
    {
        REQUIRE(scripts.reload());
        CHECK(scripts.lua()["counter"].get<int>() == 2);
        CHECK(scripts.systemCount() == 1);
    }

    SECTION("a missing script makes the config fail")
    {
        const auto bad = directory.write("bad.json",
            R"({"directory": "scripts", "scripts": ["missing.lua"]})");
        CHECK_THROWS_WITH(scripts.loadConfigFile(bad),
            Catch::Matchers::ContainsSubstring("missing.lua"));
    }
}

TEST_CASE("Destroying the engine releases its Lua data in the registry",
    "[scripting]")
{
    RType::Registry registry;
    {
        RType::ScriptEngine scripts(registry);
        RType::addScriptSystem(registry, scripts);
        REQUIRE(scripts.loadString("world.data(world.spawn()).x = 1"));
    }
    for (const auto &data : registry.getComponents<RType::ScriptData>())
        CHECK_FALSE(data.has_value());
}

TEST_CASE("The example wave script runs a small game", "[scripting]")
{
    RType::Registry registry;
    registerCoreComponents(registry);
    RType::ScriptEngine scripts(registry);
    RType::addScriptSystem(registry, scripts);
    scripts.setScriptDirectory(EXAMPLE_SCRIPT_DIRECTORY);
    REQUIRE(scripts.loadFile(
        std::filesystem::path(EXAMPLE_SCRIPT_DIRECTORY) / "waves.lua"));

    scripts.setDeltaTime(0.1f);
    std::size_t alive = 0;
    for (int frame = 0; frame < 30; ++frame) {
        registry.runSystems();
    }
    for (const auto &transform : registry.getComponents<RType::Transform2D>()) {
        if (transform.has_value()) {
            CHECK(transform->position.x < 1280.f);
            ++alive;
        }
    }

    CHECK(scripts.lastError().empty());
    CHECK(scripts.lua()["waves_spawned"].get<int>() == 2);
    CHECK(alive == 8);
}

TEST_CASE("Script components run scripts on a single entity", "[scripting]")
{
    TempDirectory directory;
    auto _ = directory.write("mover.lua", R"(
        local mover = {}
        function mover.on_start(e)
            world.data(e).starts = (world.data(e).starts or 0) + 1
        end
        function mover.on_update(e, dt)
            local t = world.get(e, "Transform2D")
            t.position.x = t.position.x + 10 * dt
        end
        return mover
    )");
    _ = directory.write("counter.lua", R"(
        return {
            on_update = function(e, dt)
                world.data(e).frames = (world.data(e).frames or 0) + 1
            end
        }
    )");
    RType::Registry registry;
    registerCoreComponents(registry);
    RType::ScriptEngine scripts(registry);
    RType::addScriptSystem(registry, scripts);
    scripts.setScriptDirectory(directory.path());
    const auto scripted = registry.spawnEntity();
    const auto plain = registry.spawnEntity();
    registry.emplaceComponent<RType::Transform2D>(scripted);
    registry.emplaceComponent<RType::Transform2D>(plain);
    registry.addComponent<RType::ScriptComponent>(
        scripted, RType::ScriptComponent {{"mover.lua", "counter.lua"}});

    scripts.setDeltaTime(1.f);
    for (int frame = 0; frame < 3; ++frame)
        registry.runSystems();

    const auto &transforms = registry.getComponents<RType::Transform2D>();
    CHECK(transforms[scripted]->position.x == 30.f);
    CHECK(transforms[plain]->position.x == 0.f);
    REQUIRE(scripts.loadString(
        "e = " + std::to_string(static_cast<RType::entity_id_type>(scripted)) +
        "\nstarts = world.data(e).starts\nframes = world.data(e).frames"));
    CHECK(scripts.lua()["starts"].get<int>() == 1);
    CHECK(scripts.lua()["frames"].get<int>() == 3);
    CHECK(scripts.lastError().empty());
}

TEST_CASE("Scripts attach entity scripts with world.add", "[scripting]")
{
    TempDirectory directory;
    auto _ = directory.write("tag.lua", R"(
        return { on_start = function(e) world.data(e).tagged = true end }
    )");
    RType::Registry registry;
    RType::ScriptEngine scripts(registry);
    RType::addScriptSystem(registry, scripts);
    scripts.setScriptDirectory(directory.path());

    REQUIRE(scripts.loadString(R"(
        e = world.spawn()
        paths = world.add(e, "Script", {"tag.lua"})
    )"));
    registry.runSystems();
    REQUIRE(scripts.loadString("tagged = world.data(e).tagged"));

    CHECK(scripts.lua()["paths"][1].get<std::string>() == "tag.lua");
    CHECK(scripts.lua()["tagged"].get<bool>());
}

TEST_CASE("A failing entity script only stops on that entity", "[scripting]")
{
    TempDirectory directory;
    auto _ = directory.write("fragile.lua", R"(
        return {
            on_update = function(e, dt)
                world.data(e).calls = (world.data(e).calls or 0) + 1
                if world.data(e).broken then error("boom") end
            end
        }
    )");
    _ = directory.write("no_table.lua", "local x = 1");
    RType::Registry registry;
    RType::ScriptEngine scripts(registry);
    RType::addScriptSystem(registry, scripts);
    scripts.setScriptDirectory(directory.path());

    REQUIRE(scripts.loadString(R"(
        broken = world.spawn()
        world.data(broken).broken = true
        world.add(broken, "Script", "fragile.lua")
        fine = world.spawn()
        world.add(fine, "Script", "fragile.lua")
    )"));
    registry.runSystems();
    registry.runSystems();
    REQUIRE(scripts.loadString(R"(
        broken_calls = world.data(broken).calls
        fine_calls = world.data(fine).calls
    )"));

    CHECK(scripts.lua()["broken_calls"].get<int>() == 1);
    CHECK(scripts.lua()["fine_calls"].get<int>() == 2);
    CHECK_THAT(scripts.lastError(),
        Catch::Matchers::ContainsSubstring("fragile.lua' on_update"));

    SECTION("a script must return a table")
    {
        REQUIRE(scripts.loadString(
            "world.add(world.spawn(), 'Script', 'no_table.lua')"));
        registry.runSystems();
        CHECK_THAT(scripts.lastError(),
            Catch::Matchers::ContainsSubstring("must return a table"));
    }
}

TEST_CASE(
    "ScriptEngine rejects actions that do not fit the mask", "[scripting]")
{
    RType::Registry registry;
    RType::ScriptEngine scripts(registry);

    REQUIRE_THROWS_AS(scripts.defineAction("TooBig", 32), std::out_of_range);
}

TEST_CASE("Script components attached before the engine starts are kept",
    "[scripting]")
{
    TempDirectory directory;
    auto _ = directory.write("mover.lua", R"(
        return {
            on_start = function(e) world.data(e).starts = 1 end,
            on_update = function(e, dt)
                local t = world.get(e, "Transform2D")
                t.position.x = t.position.x + 10 * dt
            end
        }
    )");
    RType::Registry registry;
    registerCoreComponents(registry);
    registry.registerComponent<RType::ScriptComponent>();
    const auto scripted = registry.spawnEntity();
    registry.emplaceComponent<RType::Transform2D>(scripted);
    registry.addComponent<RType::ScriptComponent>(scripted,
        RType::ScriptComponent {.scripts = {"mover.lua"}, .started = 1});

    RType::ScriptEngine scripts(registry);
    RType::addScriptSystem(registry, scripts);
    scripts.setScriptDirectory(directory.path());
    scripts.setDeltaTime(1.f);
    registry.runSystems();

    REQUIRE(registry.getComponents<RType::ScriptComponent>()[scripted]);
    CHECK(registry.getComponents<RType::Transform2D>()[scripted]->position.x ==
        10.f);
    REQUIRE(scripts.loadString("starts = world.data(" +
        std::to_string(static_cast<RType::entity_id_type>(scripted)) +
        ").starts"));
    CHECK(scripts.lua()["starts"].get<int>() == 1);
    CHECK(scripts.lastError().empty());
}

TEST_CASE("Scripts add and edit sprites", "[scripting]")
{
    RType::Registry registry;
    registerCoreComponents(registry);
    RType::ScriptEngine scripts(registry);

    REQUIRE(scripts.loadString(R"(
        ship = world.spawn()
        world.add(ship, "Sprite", Sprite.new(7, 2))
        local sprite = world.get(ship, "Sprite")
        sprite.rect_width = 32
        sprite.origin_x = 0.25
        sprite.a = 128
        z = sprite.z_index
        default_alpha = Sprite.new().a
    )"));

    const auto id = scripts.lua()["ship"].get<RType::entity_id_type>();
    const auto &sprite = registry.getComponents<RType::Sprite>()[id];
    REQUIRE(sprite.has_value());
    CHECK(sprite->assetId == 7);
    CHECK(sprite->zIndex == 2);
    CHECK(sprite->rectWidth == 32);
    CHECK(sprite->originX == 0.25f);
    CHECK(sprite->a == 128);
    CHECK(scripts.lua()["z"].get<int>() == 2);
    CHECK(scripts.lua()["default_alpha"].get<int>() == 255);
}

TEST_CASE(
    "Sprite colors are clamped and the rect constructor works", "[scripting]")
{
    RType::Registry registry;
    registerCoreComponents(registry);
    RType::ScriptEngine scripts(registry);

    REQUIRE(scripts.loadString(R"(
        local sprite = Sprite.new(1, 0, 0, 32, 16)
        sprite.a = 300
        sprite.r = -5
        sprite.g = 12
        width, height, z = sprite.rect_width, sprite.rect_height, sprite.z_index
        a, r, g = sprite.a, sprite.r, sprite.g
    )"));

    CHECK(scripts.lua()["width"].get<int>() == 32);
    CHECK(scripts.lua()["height"].get<int>() == 16);
    CHECK(scripts.lua()["z"].get<int>() == 0);
    CHECK(scripts.lua()["a"].get<int>() == 255);
    CHECK(scripts.lua()["r"].get<int>() == 0);
    CHECK(scripts.lua()["g"].get<int>() == 12);
}

TEST_CASE(
    "Scripts handle components whose storage is not registered", "[scripting]")
{
    RType::Registry registry;
    RType::ScriptEngine scripts(registry);

    REQUIRE(scripts.loadString(R"(
        e = world.spawn()
        had = world.has(e, "Sprite")
        got = world.get(e, "Sprite")
        world.remove(e, "Sprite")
        visited = 0
        world.each({"Sprite"}, function() visited = visited + 1 end)
        world.add(e, "Sprite", Sprite.new(4))
        has_now = world.has(e, "Sprite")
    )"));

    CHECK_FALSE(scripts.lua()["had"].get<bool>());
    CHECK_FALSE(scripts.lua()["got"].valid());
    CHECK(scripts.lua()["visited"].get<int>() == 0);
    CHECK(scripts.lua()["has_now"].get<bool>());
    REQUIRE(registry.isRegistered<RType::Sprite>());
    const auto id = scripts.lua()["e"].get<RType::entity_id_type>();
    CHECK(registry.getComponents<RType::Sprite>()[id]->assetId == 4);
    CHECK(scripts.lastError().empty());
}
