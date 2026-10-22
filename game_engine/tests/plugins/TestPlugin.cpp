#include "library/PluginManager.h"
#include "Module.h"
#include "ecs/SparseArray.h"
#include "TestDependencyComponent.h"
#include "TestPluginComponent.h"

#include <iterator>
#include <stdexcept>

namespace {
    constexpr const char *components[] = {"TestPluginComponent"};
    constexpr const char *systems[] = {"TestPluginSystem"};
    constexpr const char *dependencies[] = {"test_dependency"};

    void initialize(RType::Registry *registry)
    {
    registry->registerComponent<TestPluginComponent>();
    registry->addSystem<TestPluginComponent>([](auto &values) {
        for (auto &value : values) {
            if (value)
                ++value->value;
        }
    });
}
    void shutdown(RType::Registry *registry)
    {
    }

    const Engine::Plugin::Descriptor descriptor{
        .abiVersion = Engine::Plugin::ABI_VERSION,
        .name = "test_plugin",
        .components = components,
        .componentCount = std::size(components),
        .systems = systems,
        .systemCount = std::size(systems),
        .dependencies = dependencies,
        .dependencyCount = std::size(dependencies),
        .initialize = initialize,
        .shutdown = shutdown
    };

    RType::ModuleStatus registerTestModule(RType::Registry &registry)
    {
        try {
            registry.getComponents<TestDependencyComponent>();
        } catch (const std::out_of_range &) {
            return RType::ModuleStatus::InvalidRegistry;
        }
        registry.registerComponent<TestPluginComponent>();
        registry.addSystem<TestPluginComponent>([](auto &values) {
            for (auto &value : values) {
                if (value)
                    ++value->value;
            }
        });
        return RType::ModuleStatus::Success;
    }

    const RType::ModuleDescriptor moduleDescriptor{
        .name = "test_module",
        .apiVersion = {RType::MODULE_API_MAJOR, RType::MODULE_API_MINOR},
        .dependencies = {{"test_dependency_module",
            {RType::MODULE_API_MAJOR, RType::MODULE_API_MINOR}}},
        .registerModule = registerTestModule
    };
}

ENGINE_PLUGIN_EXPORT const Engine::Plugin::Descriptor *
engine_plugin_descriptor()
{
    return &descriptor;
}

extern "C" MODULE_EXPORT const RType::ModuleDescriptor *MODULE_CALL
RTypeLibEntryPoint()
{
    return &moduleDescriptor;
}
