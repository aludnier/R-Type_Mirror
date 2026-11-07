#include "LifetimeModule.h"

#include "components/Lifetime.h"
#include "components/Timer.h"
#include "components/TimerEvent.h"
#include "components/TimerSettings.h"
#include "systems/LifetimeSystem.h"
#include "systems/TimerSystem.h"

namespace {
RType::ModuleStatus registerLifetimeModule(RType::Registry &registry)
{
    registry.registerComponent<RType::Lifetime::Lifetime>();
    registry.registerComponent<RType::Lifetime::Timer>();
    registry.registerComponent<RType::Lifetime::TimerEvents>();
    registry.registerComponent<RType::Lifetime::TimerSettings>();
    RType::Lifetime::LifetimeSystem::addSystem(registry);
    RType::Lifetime::TimerSystem::addSystem(registry);
    return RType::ModuleStatus::Success;
}
} // namespace

extern "C" MODULE_EXPORT const RType::ModuleDescriptor *MODULE_CALL
RTypeLibEntryPoint()
{
    static const RType::ModuleDescriptor descriptor {
        .name = "lifetime",
        .apiVersion = {RType::MODULE_API_MAJOR, RType::MODULE_API_MINOR},
        .dependencies = {},
        .registerModule = registerLifetimeModule,
    };
    return &descriptor;
}
