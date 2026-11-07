#include "TimerSystem.h"

#include <cmath>

#include "TimerClock.h"

namespace RType::Lifetime {

void TimerSystem::update(SparseArray<Timer> &timers,
    SparseArray<TimerEvents> &events, SparseArray<TimerSettings> &settings)
{
    clearEvents(events);
    const auto dt = TimerClock::deltaTime(settings);
    for (std::size_t id = 0; id < timers.size() && id < events.size(); ++id) {
        if (timers[id] && events[id]) {
            updateTimer(*timers[id], *events[id], dt);
        }
    }
}

void TimerSystem::addSystem(Registry &registry)
{
    registry.addSystem<Timer, TimerEvents, TimerSettings>(
        [](auto &timers, auto &events, auto &settings) {
            TimerSystem::update(timers, events, settings);
        });
}

void TimerSystem::clearEvents(SparseArray<TimerEvents> &events)
{
    for (auto &entry : events) {
        if (entry) {
            entry->events.clear();
        }
    }
}

void TimerSystem::updateTimer(Timer &timer, TimerEvents &events, float dt)
{
    if (timer.fired || !std::isfinite(timer.duration) ||
        timer.duration < 0.0f) {
        return;
    }
    timer.elapsed += dt;
    if (timer.elapsed >= timer.duration) {
        timer.fired = true;
        events.events.push_back(TimerEvent {.name = timer.eventName});
    }
}

} // namespace RType::Lifetime
