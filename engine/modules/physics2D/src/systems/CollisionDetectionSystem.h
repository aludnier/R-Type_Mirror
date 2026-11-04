/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** CollisionDetectionSystem.h
*/

#ifndef PHYSICS2D_COLLISIONDETECTIONSYSTEM_H
    #define PHYSICS2D_COLLISIONDETECTIONSYSTEM_H

#include "Contact.h"
#include "ecs/Registry.h"

namespace RType::Physics2D {
/** Overlap tests (layer/mask filtered); fills `contacts` every frame. */
void addCollisionDetectionSystem(
    Registry &registry, const SharedContacts &contacts);
} // namespace RType::Physics2D

#endif // PHYSICS2D_COLLISIONDETECTIONSYSTEM_H
