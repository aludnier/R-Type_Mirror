/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** CollisionDetectionSystem.h
*/

#ifndef PHYSICS2D_COLLISIONDETECTIONSYSTEM_H
    #define PHYSICS2D_COLLISIONDETECTIONSYSTEM_H

#include "ecs/Registry.h"
#include "Contact.h"

namespace RType::Physics2D {
    /** Overlap tests (layer/mask filtered); fills `contacts` every frame. */
    void addCollisionDetectionSystem(Registry &registry, const SharedContacts &contacts);
}

#endif // PHYSICS2D_COLLISIONDETECTIONSYSTEM_H
