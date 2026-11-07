#ifndef CONTROLLABLESYSTEM_H
#define CONTROLLABLESYSTEM_H

#include "components/Action.h"
#include "ecs/Registry.h"

namespace RType {
/** @brief The action that moves an entity in each direction. */
struct MovementActions {
    ActionId up = 0;
    ActionId down = 1;
    ActionId left = 2;
    ActionId right = 3;
};

/** Registers movement driven by Controllable action masks. */
void addControllableSystem(
    Registry &registry, float speed, float dt, MovementActions actions);
} // namespace RType

#endif
