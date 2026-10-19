#include "library/PluginManager.h"
#include "TestPluginComponent.h"

#include <iterator>

namespace {
    constexpr const char *components[] = {"TestPluginComponent"};
    constexpr const char *systems[] = {"TestPluginSystem"};

    void initialize(RType::Registry *registry)
    {
        registry->registerComponent<TestPluginComponent>();
        registry->addSystem<TestPluginComponent>(
            [](RType::storage_t<TestPluginComponent> &storage) {
                for (auto &component : storage) {
                    if (component)
                        ++component->value;
                }
            });
    }

    const RType::Plugin::Descriptor descriptor{
        .abiVersion = RType::Plugin::ABI_VERSION,
        .name = "test_plugin",
        .components = components,
        .componentCount = std::size(components),
        .systems = systems,
        .systemCount = std::size(systems),
        .initialize = initialize
    };
}

RTYPE_PLUGIN_EXPORT const RType::Plugin::Descriptor *rtype_plugin_descriptor()
{
    return &descriptor;
}
