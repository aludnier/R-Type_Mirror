/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Hitbox component header
*/

#ifndef HITBOX_H
#define HITBOX_H

#include "math/Vector2.h"

namespace RType {

/**
 * @brief Simple 2D bounding box for collision detection.
 */
struct Hitbox {
    Vector2 size {32.0f, 32.0f};

    constexpr Hitbox() = default;

    constexpr explicit Hitbox(Vector2 size) :
        size(size)
    {
    }

    constexpr Hitbox(float width, float height) :
        size {width, height}
    {
    }

    constexpr bool operator==(const Hitbox &o) const = default;
};

} // namespace RType

namespace Game {
using Hitbox = RType::Hitbox;
}

#endif // HITBOX_H
