#include "LifetimeModule.h"

#include "components/Lifetime.h"
#include "components/Timer.h"
#include "components/TimerEvent.h"
#include "components/TimerSettings.h"
#include "reflection/Reflection.h"
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

// TimerEvents is rebuilt every frame: nothing to edit or save.
RType::ModuleStatus reflectLifetimeModule(RType::Reflection &reflection)
{
    using namespace RType::Lifetime;
    reflection.reflect<Lifetime>("Lifetime")
        .field("duration", &Lifetime::duration)
        .field("elapsed", &Lifetime::elapsed);
    reflection.reflect<Timer>("Timer")
        .field("duration", &Timer::duration)
        .field("elapsed", &Timer::elapsed)
        .field("eventName", &Timer::eventName)
        .field("fired", &Timer::fired);
    reflection.reflect<TimerSettings>("TimerSettings")
        .field("dt", &TimerSettings::dt);
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
        .registerReflection = reflectLifetimeModule,
    };
    return &descriptor;
}
