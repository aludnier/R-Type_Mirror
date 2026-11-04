#include <cstdint>

#include "components/Controllable.h"
#include "components/Transform2D.h"
#include "ecs/SparseArray.h"
#include "ecs/Zipper.h"
#include "systems/ControllableSystem.h"

namespace RType {
namespace {
    bool isActive(ActionMask mask, ActionId action)
    {
        return (mask & (ActionMask {1} << action)) != 0;
    }

    Vector2 directionFrom(ActionMask mask, const MovementActions &actions)
    {
        return {static_cast<float>(isActive(mask, actions.right)) -
                static_cast<float>(isActive(mask, actions.left)),
            static_cast<float>(isActive(mask, actions.down)) -
                static_cast<float>(isActive(mask, actions.up))};
    }
} // namespace

void addControllableSystem(
    Registry &registry, float speed, float dt, MovementActions actions)
{
    registry.addSystem<Transform2D, Controllable>(
        [speed, dt](auto &transforms, auto &controllables) {
            for (auto [transform, controllable] :
                Zipper(transforms, controllables)) {
                transform.position +=
                    directionFrom(controllable.actionMask, actions) *
                    (speed * dt);
            }
        });
}
} // namespace RType
