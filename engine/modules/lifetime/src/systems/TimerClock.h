#ifndef LIFETIME_TIMER_CLOCK_H
#define LIFETIME_TIMER_CLOCK_H

#include "components/TimerSettings.h"
#include "ecs/SparseArray.h"

namespace RType::Lifetime {

class TimerClock {
public:
    static float deltaTime(SparseArray<TimerSettings> &settings);
};

} // namespace RType::Lifetime

#endif
