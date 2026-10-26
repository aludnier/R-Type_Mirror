/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** MovementSystem.cpp
*/

#include "MovementSystem.h"
#include <algorithm>
#include "../components/PhysicsSettings2D.h"
#include "../components/Rigidbody2D.h"
#include "components/Transform2D.h"
#include "ecs/SparseArray.h"
#include "ecs/Zipper.h"

namespace RType::Physics2D {
namespace {
    void applyForces(Rigidbody2D &rb, const PhysicsSettings2D &s)
    {
        rb.velocity +=
            (s.gravity * rb.gravityScale + rb.force() / rb.mass) * s.dt;
    }

    void applyDrag(Rigidbody2D &rb, float dt)
    {
        rb.velocity *= std::max(0.f, 1.f - rb.drag * dt);
    }

    void integrateVelocity(
        const Rigidbody2D &rb, Transform2D &transform, float dt)
    {
        transform.position += rb.velocity * dt;
    }

    void stepBody(
        Rigidbody2D &rb, Transform2D &transform, const PhysicsSettings2D &s)
    {
        if (!rb.isStatic) {
            if (!rb.isKinematic && rb.mass > 0.f) {
                applyForces(rb, s);
                applyDrag(rb, s.dt);
            }
            integrateVelocity(rb, transform, s.dt);
        }
        rb.clearForce();
    }
} // namespace

void addMovementSystem(Registry &registry)
{
    registry.addSystem<Transform2D, Rigidbody2D, PhysicsSettings2D>(
        [](auto &transforms, auto &bodies, auto &settings) {
            const auto &s = settingsOf(settings);
            for (auto [transform, body] : Zipper(transforms, bodies)) {
                stepBody(body, transform, s);
            }
        });
}
} // namespace RType::Physics2D
