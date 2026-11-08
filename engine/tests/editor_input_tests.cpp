#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

#include "TempConfigFile.hpp"
#include "ecs/Registry.h"
#include "editor/EditorContext.h"
#include "editor/panels/InputPanel.h"
#include "library/InputManager.h"
#include "library/Keys.h"
#include "reflection/Reflection.h"

using RType::Editor::InputPanel;

namespace {
struct Fixture {
    RType::Registry registry;
    RType::Reflection reflection;
    InputManager input;
    RType::Editor::EditorContext context {
        .registry = registry, .reflection = reflection, .input = &input};
};
} // namespace

TEST_CASE("Input panel adds actions with unique names", "[editor][input]")
{
    Fixture fixture;

    CHECK_FALSE(InputPanel::addAction(fixture.context, "Jump").has_value());
    CHECK(fixture.input.actions().contains("Jump"));
    CHECK(InputPanel::addAction(fixture.context, "Jump").has_value());
    CHECK(InputPanel::addAction(fixture.context, "").has_value());
    CHECK(fixture.input.actions().size() == 1);
}

TEST_CASE("Input panel reports a full action mask", "[editor][input]")
{
    Fixture fixture;

    for (std::size_t i = 0; i < RType::MAX_ACTIONS; ++i)
        fixture.input.addAction("Action" + std::to_string(i));

    CHECK(InputPanel::addAction(fixture.context, "TooMany").has_value());
}

TEST_CASE("Input panel renames an action", "[editor][input]")
{
    Fixture fixture;
    const RType::ActionId jump = fixture.input.addAction("Jump");
    fixture.input.addAction("Shoot");

    CHECK_FALSE(
        InputPanel::renameAction(fixture.context, "Jump", "Hop").has_value());
    CHECK(fixture.input.actions().at("Hop") == jump);
    CHECK(InputPanel::renameAction(fixture.context, "Hop", "Shoot"));
    CHECK(InputPanel::renameAction(fixture.context, "Hop", ""));
    CHECK(InputPanel::renameAction(fixture.context, "Unknown", "Other"));
}

TEST_CASE("Input panel saves to a file that loads back", "[editor][input]")
{
    Fixture fixture;
    const RType::ActionId jump = fixture.input.addAction("Jump");
    fixture.input.bind(jump, RType::Key::Space);
    TempConfigFile config("{}");

    CHECK_FALSE(InputPanel::save(fixture.context, config.path()).has_value());
    CHECK(fixture.context.inputPath == config.path());

    InputManager loaded;
    loaded.loadActions(config.path());
    CHECK(loaded.actions() == fixture.input.actions());
    CHECK(loaded.bindings(jump) == fixture.input.bindings(jump));
}

TEST_CASE(
    "Input panel keeps the input file when saving fails", "[editor][input]")
{
    Fixture fixture;
    const std::filesystem::path missing =
        std::filesystem::temp_directory_path() / "rtype_no_such_dir" /
        "in.json";

    CHECK(InputPanel::save(fixture.context, missing).has_value());
    CHECK(InputPanel::save(fixture.context, "").has_value());
    CHECK_FALSE(fixture.context.inputPath.has_value());
}

TEST_CASE("Input panel needs an input manager", "[editor][input]")
{
    RType::Registry registry;
    RType::Reflection reflection;
    RType::Editor::EditorContext context {
        .registry = registry, .reflection = reflection};

    CHECK(InputPanel::addAction(context, "Jump").has_value());
    CHECK(InputPanel::renameAction(context, "Jump", "Hop").has_value());
    CHECK(InputPanel::save(context, "input.json").has_value());
}

TEST_CASE("Input panel captures the first new input", "[editor][input]")
{
    const std::unordered_set<std::string> before {"KEY_CODE_65"};

    CHECK_FALSE(InputPanel::newInput(before, before).has_value());
    CHECK_FALSE(InputPanel::newInput(before, {}).has_value());
    CHECK(InputPanel::newInput(before, {"KEY_CODE_65", "KEY_CODE_66"}) ==
        "KEY_CODE_66");
    CHECK(InputPanel::newInput({}, {"KEY_CODE_70", "KEY_CODE_66"}) ==
        "KEY_CODE_66");
}

TEST_CASE("Input panel shows readable key names", "[editor][input]")
{
    using RType::Key::A;
    using RType::Key::F5;

    CHECK(InputPanel::inputLabel(InputManager::keyCodeId(A)) == "A");
    CHECK(InputPanel::inputLabel(InputManager::keyCodeId(F5)) == "F5");
    CHECK(InputPanel::inputLabel(InputManager::keyCodeId(RType::Key::Space)) ==
        "Space");
    CHECK(InputPanel::inputLabel(InputManager::keyCodeId(RType::Key::Up)) ==
        "Up");
    CHECK(InputPanel::inputLabel(
              InputManager::keyCodeId(RType::Key::Keypad3)) == "Keypad 3");
    CHECK(InputPanel::inputLabel(InputManager::keyCodeId(999)) == "Key 999");
    CHECK(InputPanel::inputLabel("MOUSE_LEFT") == "MOUSE_LEFT");
}
