/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** OverlapTests.h
*/

#ifndef PHYSICS2D_OVERLAPTESTS_H
    #define PHYSICS2D_OVERLAPTESTS_H

#include "Shape.h"

namespace RType::Physics2D {
    bool circleCircle(const Shape &a, const Shape &b, Vector2 &normal, float &penetration);
    bool boxBox(const Shape &a, const Shape &b, Vector2 &normal, float &penetration);
    bool boxCircle(const Shape &box, const Shape &circle, Vector2 &normalBoxToCircle, float &penetration);
    /** Dispatches on shape types; normal points from a to b. */
    bool overlap(const Shape &a, const Shape &b, Vector2 &normal, float &penetration);
}

#endif // PHYSICS2D_OVERLAPTESTS_H
