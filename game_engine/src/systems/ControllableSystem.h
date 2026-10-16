#ifndef CONTROLLABLESYSTEM_H
#define CONTROLLABLESYSTEM_H

#include "ecs/Registry.h"

namespace RType {
    /** Registers movement driven by Controllable action masks. */
    void addControllableSystem(Registry &registry, float speed, float dt);
}

#endif