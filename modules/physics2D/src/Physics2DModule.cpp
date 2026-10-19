/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Physics2DModule.cpp
*/

#include "ecs/SparseArray.h"
#include "Physics2DModule.h"
#include "components/Rigidbody2D.h"
#include "components/BoxCollider2D.h"
#include "components/CircleCollider2D.h"
#include "components/CollisionEvents2D.h"
#include "components/PhysicsSettings2D.h"
#include "systems/MovementSystem.h"
#include "systems/CollisionDetectionSystem.h"
#include "systems/CollisionResponseSystem.h"

namespace {
    void registerComponents(RType::Registry &registry) {
        using namespace RType::Physics2D;
        registry.registerComponent<Rigidbody2D>();
        registry.registerComponent<BoxCollider2D>();
        registry.registerComponent<CircleCollider2D>();
        registry.registerComponent<CollisionEvents2D>();
        registry.registerComponent<PhysicsSettings2D>();
    }

    void registerSystems(RType::Registry &registry) {
        using namespace RType::Physics2D;
        const auto contacts = std::make_shared<ContactBuffer>();
        addMovementSystem(registry);
        addCollisionDetectionSystem(registry, contacts);
        addCollisionResponseSystem(registry, contacts);
    }

    RType::ModuleStatus registerPhysics2DModule(RType::Registry &registry) {
        registerComponents(registry);
        registerSystems(registry);
        return RType::ModuleStatus::Success;
    }
}

extern "C" MODULE_EXPORT const RType::ModuleDescriptor *MODULE_CALL
RTypeLibEntryPoint() {
    static const RType::ModuleDescriptor descriptor{
        .name = "physics2d",
        .apiVersion = {RType::MODULE_API_MAJOR, RType::MODULE_API_MINOR},
        .dependencies = {},
        .registerModule = registerPhysics2DModule,
    };
    return &descriptor;
}
