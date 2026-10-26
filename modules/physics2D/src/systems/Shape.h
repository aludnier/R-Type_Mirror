/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Shape.h
*/

#ifndef PHYSICS2D_SHAPE_H
    #define PHYSICS2D_SHAPE_H

#include <cstdint>
#include "ecs/Entity.h"
#include "math/Vector2.h"

namespace RType::Physics2D {
    /** World-space collider snapshot used by the collision systems. */
    struct Shape {
        entity_id_type id{};
        Vector2 center;
        Vector2 half;
        float radius{};
        bool isBox{};
        bool isTrigger{};
        std::uint32_t layer{};
        std::uint32_t mask{};
    };

    /** True if both layer/mask filters accept each other. */
    inline bool canCollide(const Shape &a, const Shape &b) {
        return a.layer & b.mask && b.layer & a.mask;
    }
}

#endif // PHYSICS2D_SHAPE_H
