/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** BoxCollider2D.h
*/

#ifndef PHYSICS2D_BOXCOLLIDER2D_H
    #define PHYSICS2D_BOXCOLLIDER2D_H

#include <cstdint>
#include "math/Vector2.h"

namespace RType::Physics2D {
    struct BoxCollider2D {
        Vector2 size{1.f, 1.f};
        Vector2 offset{};
        bool isTrigger = false;
        std::uint32_t layer = 1;
        std::uint32_t mask = 0xFFFFFFFFu;
    };
}

#endif // PHYSICS2D_BOXCOLLIDER2D_H
