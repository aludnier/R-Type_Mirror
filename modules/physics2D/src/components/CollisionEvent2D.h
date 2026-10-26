/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** CollisionEvent2D.h
*/

#ifndef PHYSICS2D_COLLISIONEVENT2D_H
#define PHYSICS2D_COLLISIONEVENT2D_H

#include "ecs/Entity.h"
#include "math/Vector2.h"

namespace RType::Physics2D {
enum class CollisionKind { Enter, Exit };

struct CollisionEvent2D {
    Entity other;
    bool isTrigger = false;
    Vector2 normal{};
    CollisionKind kind = CollisionKind::Enter;

    explicit CollisionEvent2D(Entity other, bool isTrigger, Vector2 normal,
                              CollisionKind kind) : other(other),
                                                    isTrigger(isTrigger),
                                                    normal(normal), kind(kind)
    {
    }
};
} // namespace RType::Physics2D

#endif // PHYSICS2D_COLLISIONEVENT2D_H
