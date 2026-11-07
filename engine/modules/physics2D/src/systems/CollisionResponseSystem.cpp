/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** CollisionResponseSystem.cpp
*/

#include "CollisionResponseSystem.h"
#include <memory>
#include "../components/PhysicsSettings2D.h"
#include "../components/Rigidbody2D.h"
#include "CollisionEventEmitter.h"
#include "components/Transform2D.h"
#include "ecs/SparseArray.h"

namespace RType::Physics2D {
namespace {
    template<typename Bodies>
    bool isMovable(Bodies &bodies, entity_id_type id)
    {
        return id < bodies.size() && bodies[id].has_value() &&
            !bodies[id]->isStatic && !bodies[id]->isKinematic &&
            bodies[id]->mass > 0.f;
    }

    template<typename Bodies>
    float inverseMass(Bodies &bodies, entity_id_type id)
    {
        return isMovable(bodies, id) ? 1.f / bodies[id]->mass : 0.f;
    }

    template<typename Transforms>
    void separate(Transforms &transforms, const Contact &c, float wa, float wb)
    {
        const float total = wa + wb;
        transforms[c.pair.first]->position -=
            c.normal * (c.penetration * wa / total);
        transforms[c.pair.second]->position +=
            c.normal * (c.penetration * wb / total);
    }

    template<typename Bodies>
    void bounce(
        Bodies &bodies, const Contact &c, float wa, float wb, float bounciness)
    {
        const Vector2 va =
            wa > 0.f ? bodies[c.pair.first]->velocity : Vector2 {};
        const Vector2 vb =
            wb > 0.f ? bodies[c.pair.second]->velocity : Vector2 {};
        const float speed = (vb - va).dot(c.normal);
        if (speed >= 0.f)
            return;
        const float j = -(1.f + bounciness) * speed / (wa + wb);
        if (wa > 0.f)
            bodies[c.pair.first]->velocity -= c.normal * (j * wa);
        if (wb > 0.f)
            bodies[c.pair.second]->velocity += c.normal * (j * wb);
    }

    template<typename Transforms, typename Bodies>
    void resolve(Transforms &transforms, Bodies &bodies, const Contact &c,
        float bounciness)
    {
        if (c.trigger)
            return;
        const float wa = inverseMass(bodies, c.pair.first);
        const float wb = inverseMass(bodies, c.pair.second);
        if (wa + wb <= 0.f)
            return;
        separate(transforms, c, wa, wb);
        bounce(bodies, c, wa, wb, bounciness);
    }
} // namespace

void addCollisionResponseSystem(
    Registry &registry, const SharedContacts &contacts)
{
    auto emitter = std::make_shared<CollisionEventEmitter>();
    registry.addSystem<Transform2D,
        Rigidbody2D,
        CollisionEvents2D,
        PhysicsSettings2D>(
        [contacts, emitter](
            auto &transforms, auto &bodies, auto &events, auto &settings) {
            const auto &s = settingsOf(settings);
            for (const auto &c : *contacts)
                resolve(transforms, bodies, c, s.bounciness);
            emitter->emit(*contacts, events);
        });
}
} // namespace RType::Physics2D
