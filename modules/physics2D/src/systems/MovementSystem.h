/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** MovementSystem.h
*/

#ifndef PHYSICS2D_MOVEMENTSYSTEM_H
    #define PHYSICS2D_MOVEMENTSYSTEM_H

#include "ecs/Registry.h"

namespace RType::Physics2D {
    /** Gravity, forces, drag -> velocity -> Transform2D. */
    void addMovementSystem(Registry &registry);
}

#endif // PHYSICS2D_MOVEMENTSYSTEM_H
