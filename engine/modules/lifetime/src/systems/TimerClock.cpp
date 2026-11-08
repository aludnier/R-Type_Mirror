#include "TimerClock.h"

#include <cmath>

namespace RType::Lifetime {

float TimerClock::deltaTime(SparseArray<TimerSettings> &settings)
{
    for (const auto &setting : settings) {
        if (setting && std::isfinite(setting->dt) && setting->dt >= 0.0f) {
            return setting->dt;
        }
    }
    return TimerSettings {}.dt;
}

} // namespace RType::Lifetime
