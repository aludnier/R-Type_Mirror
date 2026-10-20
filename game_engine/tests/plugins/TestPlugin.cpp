#include "library/PluginManager.h"
#include "TestPluginComponent.h"

#include <iterator>

namespace {
    constexpr const char *components[] = {"TestPluginComponent"};
    constexpr const char *systems[] = {"TestPluginSystem"};
    constexpr const char *dependencies[] = {"test_dependency"};

    RType::Registry::system_id_type systemId = 0;

    void initialize(RType::Registry *registry)
    {
        registry->registerComponent<TestPluginComponent>();
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
}

ENGINE_PLUGIN_EXPORT const Engine::Plugin::Descriptor *
engine_plugin_descriptor()
{
    return &descriptor;
}
