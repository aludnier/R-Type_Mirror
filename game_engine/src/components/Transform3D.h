#ifndef TRANSFORM3D_H
#define TRANSFORM3D_H

#include "math/Vector3.h"

namespace RType {
    /** @brief 3D transform component: position, scale and Euler rotation (radians, x/y/z). */
    struct Transform3D {
        Vector3 position{0.f, 0.f, 0.f};
        Vector3 scale{1.f, 1.f, 1.f};
        Vector3 rotation{0.f, 0.f, 0.f};

        constexpr Transform3D() = default;
        constexpr Transform3D(Vector3 position, Vector3 scale = {1.f, 1.f, 1.f}, Vector3 rotation = {})
            : position(position), scale(scale), rotation(rotation) {}

        constexpr bool operator==(const Transform3D &o) const = default;
    };
}

#endif
