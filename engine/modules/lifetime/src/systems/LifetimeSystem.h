#ifndef LIFETIME_SYSTEM_H
#define LIFETIME_SYSTEM_H

#include "components/Lifetime.h"
#include "components/TimerSettings.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"

namespace RType::Lifetime {

class LifetimeSystem {
public:
    static void update(SparseArray<Lifetime> &lifetimes,
        SparseArray<TimerSettings> &settings, Registry &registry);
    static void addSystem(Registry &registry);
};

} // namespace RType::Lifetime

#endif
