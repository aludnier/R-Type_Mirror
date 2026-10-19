/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** CollisionEvents2D.h
*/

#ifndef PHYSICS2D_COLLISIONEVENTS2D_H
    #define PHYSICS2D_COLLISIONEVENTS2D_H

#include <vector>
#include "CollisionEvent2D.h"

namespace RType::Physics2D {
    /** Events of the current frame; cleared and refilled every frame. */
    struct CollisionEvents2D {
        std::vector<CollisionEvent2D> events;
    };
}

#endif // PHYSICS2D_COLLISIONEVENTS2D_H
