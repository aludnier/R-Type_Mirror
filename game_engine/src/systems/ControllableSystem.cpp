#include <cstddef>
#include <cstdint>

#include "components/Controllable.h"
#include "components/Transform2D.h"
#include "ecs/SparseArray.h"
#include "ecs/Zipper.h"
#include "library/InputManager.h"
#include "systems/ControllableSystem.h"

namespace RType {
    namespace {
        bool isActive(std::uint8_t mask, Action action) {
            const auto bit = static_cast<std::uint8_t>(1u << static_cast<std::uint8_t>(action));
            return (mask & bit) != 0;
        }

        Vector2 directionFrom(std::uint8_t mask) {
            return {
                static_cast<float>(isActive(mask, Action::MoveRight)) -
                    static_cast<float>(isActive(mask, Action::MoveLeft)),
                static_cast<float>(isActive(mask, Action::MoveDown)) -
                    static_cast<float>(isActive(mask, Action::MoveUp))
            };
        }
    }

    void addControllableSystem(Registry &registry, float speed, float dt) {
        registry.addSystem<Transform2D, Controllable>(
            [speed, dt](auto &transforms, auto &controllables) {
                for (auto [transform, controllable] : Zipper(transforms, controllables)) {
                    transform.position += directionFrom(controllable.actionMask) * (speed * dt);
                }
            });
    }
}