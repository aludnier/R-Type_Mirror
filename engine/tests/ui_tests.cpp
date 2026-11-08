#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

#include "ui/UiDocument.h"
#include "ui/UiManager.h"

namespace {
class TempUiDirectory {
public:
    TempUiDirectory()
    {
        const auto stamp =
            std::chrono::steady_clock::now().time_since_epoch().count();
        _path = std::filesystem::temp_directory_path() /
            ("rtype_ui_test_" + std::to_string(stamp));
        std::filesystem::create_directories(_path);
        _path = std::filesystem::canonical(_path);
    }

    ~TempUiDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(_path, error);
    }

    [[nodiscard]] std::filesystem::path write(
        const std::string &name, const std::string &contents) const
    {
        const auto file = _path / name;
        std::filesystem::create_directories(file.parent_path());
        std::ofstream output(file, std::ios::trunc);
        output << contents;
        if (!output)
            throw std::runtime_error("failed to write test UI file");
        return file;
    }

    [[nodiscard]] const std::filesystem::path &path() const
    {
        return _path;
    }

private:
    std::filesystem::path _path;
};
} // namespace

TEST_CASE("UI loader parses widget types and resolves nested includes", "[ui]")
{
    TempUiDirectory files;
    const auto child = files.write("parts/child.json", R"({
        "name": "child",
        "elements": [
            {"type": "text", "id": "caption", "rect": [0.1, 0.1, 0.8, 0.2], "text": "Included"}
        ]
    })");
    const auto file = files.write("main.json", R"({
        "name": "main",
        "elements": [
            {"type": "panel", "id": "panel", "rect": [0, 0, 1, 1], "children": [
                {"type": "text", "id": "title", "rect": [0, 0, 1, 0.1], "text": "Settings"}
            ]},
            {"type": "image", "id": "logo", "rect": [0, 0, 64, 64], "path": "images/logo.png"},
            {"type": "button", "id": "start", "rect": [0, 0, 0.2, 0.1], "text": "Start"},
            {"type": "input_text", "id": "player", "rect": [0, 0, 0.3, 0.1], "hint": "Pilot", "max_length": 16, "value": "Ada"},
            {"type": "checkbox", "id": "music", "rect": [0, 0, 0.1, 0.1], "checked": true},
            {"type": "slider", "id": "volume", "rect": [0, 0, 0.3, 0.1], "min": 0, "max": 1, "value": 0.5},
            {"type": "include", "name": "footer", "file": "parts/child.json", "rect": [0.2, 0.8, 0.6, 0.2]}
        ]
    })");

    const auto document = RType::UI::UiLoader::loadFile(file);
    REQUIRE(document.name == "main");
    REQUIRE(document.elements.size() == 7);
    CHECK(document.elements[0].type == RType::UI::UiElementType::Panel);
    CHECK(document.elements[1].path ==
        (files.write("images/unused", "").parent_path() / "logo.png")
            .lexically_normal()
            .string());
    CHECK(document.elements[2].type == RType::UI::UiElementType::Button);
    CHECK(document.elements[3].maxLength == 16);
    CHECK(document.elements[4].checked);
    CHECK(document.elements[5].value == 0.5f);
    REQUIRE(document.elements[6].type == RType::UI::UiElementType::Include);
    CHECK(document.elements[6].rect.x == 0.2f);
    REQUIRE(document.elements[6].children.size() == 1);
    CHECK(document.elements[6].children.front().text == "Included");
    CHECK(child.filename() == "child.json");
}

TEST_CASE("UI loader resolves assets and rejects invalid documents", "[ui]")
{
    TempUiDirectory files;
    const auto assetFile = files.write("asset.json", R"({
        "name": "assets",
        "elements": [{"type": "image", "id": "logo", "rect": [0, 0, 1, 1], "asset": "Logo"}]
    })");
    CHECK(RType::UI::UiLoader::loadFile(assetFile).elements[0].asset == "Logo");

    const auto missing = files.write("missing-include.json", R"({
        "name": "missing", "elements": [
            {"type": "include", "name": "absent", "file": "absent.json", "rect": [0, 0, 1, 1]}
        ]
    })");
    CHECK_THROWS_AS(RType::UI::UiLoader::loadFile(missing), std::runtime_error);

    const auto unknown = files.write("unknown.json", R"({
        "name": "unknown", "elements": [
            {"type": "unknown", "id": "x", "rect": [0, 0, 1, 1]}
        ]
    })");
    CHECK_THROWS_WITH(RType::UI::UiLoader::loadFile(unknown),
        Catch::Matchers::ContainsSubstring("unknown UI element type"));

    const auto cycleA = files.write("cycle-a.json", R"({
        "name": "a", "elements": [
            {"type": "include", "name": "b", "file": "cycle-b.json", "rect": [0, 0, 1, 1]}
        ]
    })");
    static_cast<void>(files.write("cycle-b.json", R"({
        "name": "b", "elements": [
            {"type": "include", "name": "a", "file": "cycle-a.json", "rect": [0, 0, 1, 1]}
        ]
    })"));
    CHECK_THROWS_WITH(RType::UI::UiLoader::loadFile(cycleA),
        Catch::Matchers::ContainsSubstring("include cycle detected"));

    CHECK_THROWS_AS(
        RType::UI::UiLoader::loadFile(files.path() / "not-found.json"),
        std::runtime_error);
}

TEST_CASE(
    "UI manager tracks visibility, values, callbacks and click events", "[ui]")
{
    TempUiDirectory files;
    static_cast<void>(files.write("parts/child.json", R"({
        "name": "child", "elements": [
            {"type": "input_text", "id": "nickname", "rect": [0, 0, 1, 0.1], "max_length": 5, "value": "Pilot"}
        ]
    })"));
    const auto file = files.write("menu.json", R"({
        "name": "menu", "elements": [
            {"type": "input_text", "id": "player", "rect": [0, 0, 0.5, 0.1], "max_length": 4, "value": "Ada"},
            {"type": "checkbox", "id": "music", "rect": [0, 0, 0.1, 0.1]},
            {"type": "slider", "id": "volume", "rect": [0, 0, 0.5, 0.1], "min": 0, "max": 1, "value": 0.25},
            {"type": "button", "id": "play", "rect": [0, 0, 0.2, 0.1]},
            {"type": "include", "name": "advanced", "file": "parts/child.json", "rect": [0, 0, 1, 0.2]}
        ]
    })");

    RType::UI::UiManager manager;
    manager.load("menu", file);
    CHECK_FALSE(manager.isVisible("menu"));
    manager.show("menu");
    CHECK(manager.isVisible("menu"));
    CHECK(manager.getText("menu/player") == "Ada");
    manager.setText("menu/player", "Grace");
    CHECK(manager.getText("menu/player") == "Grac");
    CHECK(manager.getText("menu/advanced/nickname") == "Pilot");
    manager.setText("menu/advanced/nickname", "LongName");
    CHECK(manager.getText("menu/advanced/nickname") == "LongN");
    manager.setValue("menu/music", true);
    CHECK(std::get<bool>(manager.getValue("menu/music")));
    manager.setValue("menu/volume", 0.75f);
    CHECK(std::get<float>(manager.getValue("menu/volume")) == 0.75f);

    int clickCount = 0;
    int replacementClickCount = 0;
    manager.onClick("menu/play", [&] {
        ++clickCount;
        manager.onClick(
            "menu/play", [&replacementClickCount] { ++replacementClickCount; });
    });
    manager.click("menu/play");
    CHECK(clickCount == 1);
    CHECK(replacementClickCount == 0);
    CHECK(
        manager.takeClickedEvents() == std::vector<std::string> {"menu/play"});
    CHECK(manager.takeClickedEvents().empty());
    manager.click("menu/play");
    CHECK(replacementClickCount == 1);
    manager.clearClickedEvents();
    CHECK(manager.takeClickedEvents().empty());
    manager.hide("menu");
    CHECK_FALSE(manager.isVisible("menu"));
}
