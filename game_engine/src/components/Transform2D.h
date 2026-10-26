#ifndef TRANSFORM2D_H
#define TRANSFORM2D_H

#include "math/Vector2.h"

namespace RType {
    /** @brief 2D transform component: position, scale and rotation (radians). */
    struct Transform2D {
        Vector2 position{0.f, 0.f};
        Vector2 scale{1.f, 1.f};
        float rotation = 0.f;

        constexpr Transform2D() = default;

        explicit constexpr Transform2D(Vector2 position, Vector2 scale = {1.f, 1.f}, float rotation = 0.f)
            : position(position), scale(scale), rotation(rotation) {}

        constexpr bool operator==(const Transform2D &o) const = default;
    };
}

#endif
