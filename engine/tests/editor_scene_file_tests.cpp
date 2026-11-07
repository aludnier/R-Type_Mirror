#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <system_error>

#include "components/Name.h"
#include "components/Transform2D.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"
#include "editor/EditorContext.h"
#include "editor/SceneFile.h"
#include "editor/panels/HierarchyPanel.h"
#include "library/AssetManager.h"
#include "reflection/Reflection.h"

namespace SceneFile = RType::Editor::SceneFile;
using RType::Editor::HierarchyPanel;

namespace {
struct SceneFileFixture {
    SceneFileFixture()
    {
        RType::reflectCoreComponents(reflection);
    }

    ~SceneFileFixture()
    {
        std::error_code error;
        std::filesystem::remove(file, error);
    }

    SceneFileFixture(const SceneFileFixture &) = delete;
    SceneFileFixture &operator=(const SceneFileFixture &) = delete;

    RType::Registry registry;
    RType::Reflection reflection;
    AssetManager assets;
    RType::Editor::EditorContext context {
        .registry = registry, .reflection = reflection, .assets = &assets};
    std::filesystem::path file =
        std::filesystem::temp_directory_path() / "rtype_editor_scene.json";
};
} // namespace

TEST_CASE("Editor saves, clears and opens a scene", "[editor]")
{
    SceneFileFixture fixture;
    const auto player = HierarchyPanel::createEntity(fixture.context);
    HierarchyPanel::rename(fixture.context, player, "Player");
    fixture.reflection.find("Transform2D")->add(fixture.registry, player);

    REQUIRE_FALSE(SceneFile::saveScene(fixture.context, fixture.file));
    CHECK(fixture.context.scenePath == fixture.file);

    SceneFile::newScene(fixture.context);
    CHECK(fixture.registry.entities().empty());
    CHECK_FALSE(fixture.context.selected.has_value());
    CHECK_FALSE(fixture.context.scenePath.has_value());

    HierarchyPanel::createEntity(fixture.context);
    REQUIRE_FALSE(SceneFile::openScene(fixture.context, fixture.file));

    REQUIRE(fixture.registry.entities().size() == 1);
    const auto loaded = fixture.registry.entities().front();
    CHECK(HierarchyPanel::label(fixture.registry, loaded) == "Player");
    CHECK(fixture.registry.getComponents<RType::Transform2D>()[loaded]);
    CHECK_FALSE(fixture.context.selected.has_value());
    CHECK(fixture.context.scenePath == fixture.file);
}

TEST_CASE("Opening a bad file keeps the current scene", "[editor]")
{
    SceneFileFixture fixture;
    const auto kept = HierarchyPanel::createEntity(fixture.context);

    CHECK(SceneFile::openScene(fixture.context, "missing_scene_file.json"));
    {
        std::ofstream invalid(fixture.file);
        invalid << "{not json";
    }
    CHECK(SceneFile::openScene(fixture.context, fixture.file));

    CHECK(fixture.registry.entities() == std::vector {kept});
    CHECK(fixture.context.selectedEntity() == kept);
    CHECK_FALSE(fixture.context.scenePath.has_value());
}

TEST_CASE("Saving needs an AssetManager", "[editor]")
{
    SceneFileFixture fixture;
    fixture.context.assets = nullptr;
    HierarchyPanel::createEntity(fixture.context);

    CHECK(SceneFile::saveScene(fixture.context, fixture.file));
    CHECK_FALSE(std::filesystem::exists(fixture.file));
    CHECK_FALSE(fixture.context.scenePath.has_value());
}

TEST_CASE("Opening works without an AssetManager", "[editor]")
{
    SceneFileFixture fixture;
    HierarchyPanel::createEntity(fixture.context);
    REQUIRE_FALSE(SceneFile::saveScene(fixture.context, fixture.file));
    fixture.context.assets = nullptr;

    REQUIRE_FALSE(SceneFile::openScene(fixture.context, fixture.file));
    CHECK(fixture.registry.entities().size() == 1);
}
