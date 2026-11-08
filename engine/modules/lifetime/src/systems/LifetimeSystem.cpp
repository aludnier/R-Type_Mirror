#include "LifetimeSystem.h"

#include <cmath>

#include "TimerClock.h"

namespace RType::Lifetime {

void LifetimeSystem::update(SparseArray<Lifetime> &lifetimes,
    SparseArray<TimerSettings> &settings, Registry &registry)
{
    const auto dt = TimerClock::deltaTime(settings);
    for (std::size_t id = 0; id < lifetimes.size(); ++id) {
        if (!lifetimes[id] || !std::isfinite(lifetimes[id]->duration)) {
            continue;
        }
        const auto entity = Registry::entityFromIndex(id);
        if (!registry.isAlive(entity) || registry.isPendingKill(entity)) {
            continue;
        }
        lifetimes[id]->elapsed += dt;
        if (lifetimes[id]->elapsed >= lifetimes[id]->duration) {
            registry.killEntity(entity);
        }
    }
}

void LifetimeSystem::addSystem(Registry &registry)
{
    registry.addSystem<Lifetime, TimerSettings>(
        [&registry](auto &lifetimes, auto &settings) {
            LifetimeSystem::update(lifetimes, settings, registry);
        });
}

} // namespace RType::Lifetime
