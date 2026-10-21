#include "library/PluginManager.h"
#include "Module.h"
#include "ecs/SparseArray.h"
#include "TestDependencyComponent.h"

namespace {
    void initialize(RType::Registry *registry)
    {
        registry->registerComponent<TestDependencyComponent>();
    }

    void shutdown(RType::Registry *registry)
    {
    }

    const Engine::Plugin::Descriptor descriptor{
        .abiVersion = Engine::Plugin::ABI_VERSION,
        .name = "test_dependency",
        .components = nullptr,
        .componentCount = 0,
        .systems = nullptr,
        .systemCount = 0,
        .dependencies = nullptr,
        .dependencyCount = 0,
        .initialize = initialize,
        .shutdown = shutdown
    };

    RType::ModuleStatus registerDependencyModule(RType::Registry &registry)
    {
        initialize(&registry);
        return RType::ModuleStatus::Success;
    }

    const RType::ModuleDescriptor moduleDescriptor{
        .name = "test_dependency_module",
        .apiVersion = {RType::MODULE_API_MAJOR, RType::MODULE_API_MINOR},
        .dependencies = {},
        .registerModule = registerDependencyModule
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
