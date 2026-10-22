#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

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
            if (entry.is_regular_file() &&
                filename.find(name) != std::string::npos)
                return entry.path();
        }
        throw std::runtime_error("Could not find test module library '" +
            name + "' in " + directory.string());
    }
}

TEST_CASE("PluginManager loads providers and dependencies in dependency order",
          "[plugins]") {
    RType::Registry registry;
    PluginManager manager;

    manager.loadRequiredPlugins(TEST_PLUGIN_DIRECTORY,
        {"TestPluginComponent"}, {"TestPluginSystem"}, registry);

    REQUIRE(manager.isLoaded("test_dependency"));
    REQUIRE(manager.isLoaded("test_plugin"));
    REQUIRE(registry.getComponents<TestDependencyComponent>().size() == 0);

    const auto entity = registry.spawnEntity();
    registry.emplaceComponent<TestPluginComponent>(entity, 41);
    registry.runSystems();

    REQUIRE(registry.getComponents<TestPluginComponent>()[entity]->value == 42);
}

TEST_CASE("PluginManager blocks dependency unload and safely unloads plugins",
          "[plugins]") {
    RType::Registry registry;
    PluginManager manager;
    manager.loadRequiredPlugins(TEST_PLUGIN_DIRECTORY,
        {"TestPluginComponent"}, {}, registry);

    REQUIRE_THROWS_AS(manager.unload("test_dependency"), std::runtime_error);
    REQUIRE(manager.unload("test_plugin"));
    REQUIRE_FALSE(manager.isLoaded("test_plugin"));
    REQUIRE(manager.isLoaded("test_dependency"));
    REQUIRE_NOTHROW(registry.runSystems());
    REQUIRE(manager.unload("test_dependency"));
    REQUIRE_FALSE(manager.unload("unknown_plugin"));
}

TEST_CASE("PluginManager loads ModuleDescriptor dependencies before the module",
          "[plugins][modules]") {
    PluginManager manager;
    RType::Registry registry;

    manager.loadModule(findModuleLibrary(TEST_PLUGIN_DIRECTORY, "test_plugin"),
        registry);

    REQUIRE(manager.isLoaded("test_dependency_module"));
    REQUIRE(manager.isLoaded("test_module"));
    REQUIRE(registry.getComponents<TestDependencyComponent>().size() == 0);
    REQUIRE(registry.getComponents<TestPluginComponent>().size() == 0);

    const auto entity = registry.spawnEntity();
    registry.emplaceComponent<TestPluginComponent>(entity, 41);
    registry.runSystems();

    REQUIRE(registry.getComponents<TestPluginComponent>()[entity]->value == 42);
}

TEST_CASE("PluginManager rolls back plugins when initialization throws",
          "[plugins]") {
    RType::Registry registry;
    PluginManager manager;

    REQUIRE_THROWS_WITH(manager.loadRequiredPlugins(TEST_PLUGIN_DIRECTORY,
            {"TestFailingComponent"}, {}, registry),
        "test plugin initialization failed");
    REQUIRE_FALSE(manager.isLoaded("test_failing_plugin"));
    REQUIRE_FALSE(manager.isLoaded("test_dependency"));
}

TEST_CASE("PluginManager unloads all plugins and their ECS registrations",
          "[plugins]") {
    RType::Registry registry;
    PluginManager manager;
    manager.loadRequiredPlugins(TEST_PLUGIN_DIRECTORY,
        {"TestPluginComponent"}, {}, registry);

    manager.unloadAll();

    REQUIRE_FALSE(manager.isLoaded("test_plugin"));
    REQUIRE_FALSE(manager.isLoaded("test_dependency"));
    REQUIRE_NOTHROW(registry.runSystems());
}

TEST_CASE("PluginManager rejects missing requirements and repeat configuration",
          "[plugins]") {
    RType::Registry registry;
    PluginManager missingRequirement;

    REQUIRE_THROWS_WITH(missingRequirement.loadRequiredPlugins(TEST_PLUGIN_DIRECTORY,
            {"MissingComponent"}, {}, registry),
        "No plugin provides required component 'MissingComponent'");

    PluginManager configured;
    configured.loadRequiredPlugins(
        std::filesystem::path("does-not-need-to-exist"), {}, {}, registry);
    REQUIRE_THROWS_AS(configured.loadRequiredPlugins(TEST_PLUGIN_DIRECTORY, {}, {}, registry),
        std::logic_error);
}

TEST_CASE("PluginManager reports an invalid plugin directory", "[plugins]") {
    RType::Registry registry;
    PluginManager manager;

    REQUIRE_THROWS_AS(manager.loadRequiredPlugins(
            std::filesystem::path("missing_plugin_directory"),
            {"TestPluginComponent"}, {}, registry),
        std::runtime_error);
}
