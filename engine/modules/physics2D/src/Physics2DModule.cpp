/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Physics2DModule.cpp
*/

#include "Physics2DModule.h"
#include "components/BoxCollider2D.h"
#include "components/CircleCollider2D.h"
#include "components/CollisionEvents2D.h"
#include "components/PhysicsSettings2D.h"
#include "components/Rigidbody2D.h"
#include "ecs/SparseArray.h"
#include "reflection/Reflection.h"
#include "systems/CollisionDetectionSystem.h"
#include "systems/CollisionResponseSystem.h"
#include "systems/MovementSystem.h"

namespace {
void registerComponents(RType::Registry &registry)
{
    using namespace RType::Physics2D;
    registry.registerComponent<Rigidbody2D>();
    registry.registerComponent<BoxCollider2D>();
    registry.registerComponent<CircleCollider2D>();
    registry.registerComponent<CollisionEvents2D>();
    registry.registerComponent<PhysicsSettings2D>();
}

void registerSystems(RType::Registry &registry)
{
    using namespace RType::Physics2D;
    const auto contacts = std::make_shared<ContactBuffer>();
    addMovementSystem(registry);
    addCollisionDetectionSystem(registry, contacts);
    addCollisionResponseSystem(registry, contacts);
}

RType::ModuleStatus registerPhysics2DModule(RType::Registry &registry)
{
    registerComponents(registry);
    registerSystems(registry);
    return RType::ModuleStatus::Success;
}

// CollisionEvents2D is rebuilt every frame: nothing to edit or save.
RType::ModuleStatus reflectPhysics2DModule(RType::Reflection &reflection)
{
    using namespace RType::Physics2D;
    reflection.reflect<Rigidbody2D>("Rigidbody2D")
        .field("velocity", &Rigidbody2D::velocity)
        .field("mass", &Rigidbody2D::mass)
        .field("drag", &Rigidbody2D::drag)
        .field("gravityScale", &Rigidbody2D::gravityScale)
        .field("isKinematic", &Rigidbody2D::isKinematic)
        .field("isStatic", &Rigidbody2D::isStatic);
    reflection.reflect<BoxCollider2D>("BoxCollider2D")
        .field("size", &BoxCollider2D::size)
        .field("offset", &BoxCollider2D::offset)
        .field("isTrigger", &BoxCollider2D::isTrigger)
        .field("layer", &BoxCollider2D::layer)
        .field("mask", &BoxCollider2D::mask);
    reflection.reflect<CircleCollider2D>("CircleCollider2D")
        .field("radius", &CircleCollider2D::radius)
        .field("offset", &CircleCollider2D::offset)
        .field("isTrigger", &CircleCollider2D::isTrigger)
        .field("layer", &CircleCollider2D::layer)
        .field("mask", &CircleCollider2D::mask);
    reflection.reflect<PhysicsSettings2D>("PhysicsSettings2D")
        .field("gravity", &PhysicsSettings2D::gravity)
        .field("dt", &PhysicsSettings2D::dt)
        .field("bounciness", &PhysicsSettings2D::bounciness);
    return RType::ModuleStatus::Success;
}
} // namespace

extern "C" MODULE_EXPORT const RType::ModuleDescriptor *MODULE_CALL
RTypeLibEntryPoint()
{
    static const RType::ModuleDescriptor descriptor {
        .name = "physics2d",
        .apiVersion = {RType::MODULE_API_MAJOR, RType::MODULE_API_MINOR},
        .dependencies = {},
        .registerModule = registerPhysics2DModule,
        .registerReflection = reflectPhysics2DModule,
    };
    return &descriptor;
}
