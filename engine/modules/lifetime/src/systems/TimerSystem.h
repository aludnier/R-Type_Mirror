#ifndef LIFETIME_TIMER_SYSTEM_H
#define LIFETIME_TIMER_SYSTEM_H

#include "components/Timer.h"
#include "components/TimerEvent.h"
#include "components/TimerSettings.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"

namespace RType::Lifetime {

class TimerSystem {
public:
    static void update(SparseArray<Timer> &timers,
        SparseArray<TimerEvents> &events,
        SparseArray<TimerSettings> &settings);
    static void addSystem(Registry &registry);

private:
    static void clearEvents(SparseArray<TimerEvents> &events);
    static void updateTimer(Timer &timer, TimerEvents &events, float dt);
};

} // namespace RType::Lifetime

#endif
