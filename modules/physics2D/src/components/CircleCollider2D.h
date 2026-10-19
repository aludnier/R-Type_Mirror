/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** CircleCollider2D.h
*/

#ifndef PHYSICS2D_CIRCLECOLLIDER2D_H
    #define PHYSICS2D_CIRCLECOLLIDER2D_H

#include <cstdint>
#include "math/Vector2.h"

namespace RType::Physics2D {
    struct CircleCollider2D {
        float radius = 0.5f;
        Vector2 offset{};
        bool isTrigger = false;
        std::uint32_t layer = 1;
        std::uint32_t mask = 0xFFFFFFFFu;
    };
}

#endif // PHYSICS2D_CIRCLECOLLIDER2D_H
