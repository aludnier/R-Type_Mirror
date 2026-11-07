/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** CollisionResponseSystem.h
*/

#ifndef PHYSICS2D_COLLISIONRESPONSESYSTEM_H
    #define PHYSICS2D_COLLISIONRESPONSESYSTEM_H

#include "Contact.h"
#include "ecs/Registry.h"

namespace RType::Physics2D {
/** Separates and bounces solid contacts, then writes Enter/Exit events. */
void addCollisionResponseSystem(
    Registry &registry, const SharedContacts &contacts);
} // namespace RType::Physics2D

#endif // PHYSICS2D_COLLISIONRESPONSESYSTEM_H
