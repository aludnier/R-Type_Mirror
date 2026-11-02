#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <filesystem>
#include <stdexcept>

#include "ecs/Registry.h"
#include "ecs/SparseArray.h"
#include "library/PluginManager.h"
#include "plugins/TestDependencyComponent.h"
#include "plugins/TestFailingComponent.h"
#include "plugins/TestPluginComponent.h"

namespace {
std::filesystem::path findModuleLibrary(
    const std::filesystem::path &directory, const std::string &name)
{
    for (const auto &entry : std::filesystem::directory_iterator(directory)) {
        const auto filename = entry.path().filename().string();
        if (entry.is_regular_file() && filename.find(name) != std::string::npos)
            return entry.path();
    }
    throw std::runtime_error("Could not find test module library '" + name +
        "' in " + directory.string());
}
} // namespace

TEST_CASE("PluginManager loads requested modules and dependencies in "
          "dependency order",
    "[plugins]")
{
    RType::Registry registry;
    RType::PluginManager manager;

    manager.loadRequiredModules(
        TEST_PLUGIN_DIRECTORY, {"test_module"}, registry);

    REQUIRE(manager.isLoaded("test_dependency_module"));
    REQUIRE(manager.isLoaded("test_module"));
    REQUIRE_FALSE(manager.isLoaded("test_failing_module"));
    REQUIRE(registry.getComponents<TestDependencyComponent>().size() == 0);

    const auto entity = registry.spawnEntity();
    registry.emplaceComponent<TestPluginComponent>(entity, 41);
    registry.runSystems();

    REQUIRE(registry.getComponents<TestPluginComponent>()[entity]->value == 42);
}

TEST_CASE(
    "PluginManager registers a dependency only once when it is also requested",
    "[plugins]")
{
    RType::Registry registry;
    RType::PluginManager manager;

    manager.loadRequiredModules(TEST_PLUGIN_DIRECTORY,
        {"test_module", "test_dependency_module"},
        registry);

    REQUIRE(manager.isLoaded("test_dependency_module"));
    REQUIRE(manager.isLoaded("test_module"));

    const auto entity = registry.spawnEntity();
    registry.emplaceComponent<TestPluginComponent>(entity, 41);
    registry.runSystems();

    REQUIRE(registry.getComponents<TestPluginComponent>()[entity]->value == 42);
}

TEST_CASE("PluginManager loads ModuleDescriptor dependencies before the module",
    "[plugins][modules]")
{
    RType::Registry registry;
    RType::PluginManager manager;

    manager.loadModule(
        findModuleLibrary(TEST_PLUGIN_DIRECTORY, "test_plugin"), registry);

    REQUIRE(manager.isLoaded("test_dependency_module"));
    REQUIRE(manager.isLoaded("test_module"));
    REQUIRE(registry.getComponents<TestDependencyComponent>().size() == 0);
    REQUIRE(registry.getComponents<TestPluginComponent>().size() == 0);

    const auto entity = registry.spawnEntity();
    registry.emplaceComponent<TestPluginComponent>(entity, 41);
    registry.runSystems();

    REQUIRE(registry.getComponents<TestPluginComponent>()[entity]->value == 42);
}

TEST_CASE(
    "PluginManager refuses to load the same module twice", "[plugins][modules]")
{
    RType::Registry registry;
    RType::PluginManager manager;
    const auto library =
        findModuleLibrary(TEST_PLUGIN_DIRECTORY, "test_plugin");

    manager.loadModule(library, registry);

    REQUIRE_THROWS_AS(manager.loadModule(library, registry), std::logic_error);
    REQUIRE(manager.isLoaded("test_module"));
}

TEST_CASE(
    "PluginManager refuses to bind to a second registry", "[plugins][modules]")
{
    RType::Registry first;
    RType::Registry second;
    RType::PluginManager manager;

    manager.loadModule(
        findModuleLibrary(TEST_PLUGIN_DIRECTORY, "test_plugin"), first);

    REQUIRE_THROWS_AS(
        manager.loadModule(
            findModuleLibrary(TEST_PLUGIN_DIRECTORY, "test_plugin"), second),
        std::logic_error);
}

TEST_CASE("PluginManager forgets the modules of a failed loadRequiredModules",
    "[plugins]")
{
    RType::Registry registry;
    RType::PluginManager manager;

    REQUIRE_THROWS_WITH(
        manager.loadRequiredModules(
            TEST_PLUGIN_DIRECTORY, {"test_failing_module"}, registry),
        "test plugin initialization failed");
    REQUIRE_FALSE(manager.isLoaded("test_failing_module"));
    REQUIRE_FALSE(manager.isLoaded("test_dependency_module"));
}

TEST_CASE("PluginManager forgets the modules of a failed loadModule",
    "[plugins][modules]")
{
    RType::Registry registry;
    RType::PluginManager manager;

    REQUIRE_THROWS_WITH(
        manager.loadModule(
            findModuleLibrary(TEST_PLUGIN_DIRECTORY, "test_failing"), registry),
        "test plugin initialization failed");
    REQUIRE_FALSE(manager.isLoaded("test_failing_module"));
    REQUIRE_FALSE(manager.isLoaded("test_dependency_module"));
}

TEST_CASE(
    "PluginManager keeps earlier modules when a later load fails", "[plugins]")
{
    RType::Registry registry;
    RType::PluginManager manager;

    manager.loadModule(
        findModuleLibrary(TEST_PLUGIN_DIRECTORY, "test_plugin"), registry);

    REQUIRE_THROWS_WITH(
        manager.loadModule(
            findModuleLibrary(TEST_PLUGIN_DIRECTORY, "test_failing"), registry),
        "test plugin initialization failed");
    REQUIRE_FALSE(manager.isLoaded("test_failing_module"));
    REQUIRE(manager.isLoaded("test_dependency_module"));
    REQUIRE(manager.isLoaded("test_module"));
}

TEST_CASE("PluginManager rejects missing requirements and repeat configuration",
    "[plugins]")
{
    RType::Registry registry;
    RType::PluginManager missingRequirement;

    REQUIRE_THROWS_WITH(missingRequirement.loadRequiredModules(
                            TEST_PLUGIN_DIRECTORY, {"MissingModule"}, registry),
        Catch::Matchers::ContainsSubstring("No module named 'MissingModule'"));

    RType::PluginManager configured;
    configured.loadRequiredModules(
        std::filesystem::path("does-not-need-to-exist"), {}, registry);
    REQUIRE_THROWS_AS(
        configured.loadRequiredModules(TEST_PLUGIN_DIRECTORY, {}, registry),
        std::logic_error);
}

TEST_CASE("PluginManager reports an invalid plugin directory", "[plugins]")
{
    RType::Registry registry;
    RType::PluginManager manager;

    REQUIRE_THROWS_AS(manager.loadRequiredModules(
                          std::filesystem::path("missing_plugin_directory"),
                          {"test_module"},
                          registry),
        std::runtime_error);
}
