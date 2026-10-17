/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** PhysicsSettings2D.h
*/

#ifndef PHYSICS2D_PHYSICSSETTINGS2D_H
    #define PHYSICS2D_PHYSICSSETTINGS2D_H

#include "ecs/SparseArray.h"
#include "math/Vector2.h"

namespace RType::Physics2D {
    struct PhysicsSettings2D {
        Vector2 gravity{0.f, 0.f};
        float dt = 1.f / 60.f;
        float bounciness = 0.f;
    };

    /** Returns the first settings component found, or defaults. */
    const PhysicsSettings2D &settingsOf(SparseArray<PhysicsSettings2D> &storage);
}

#endif // PHYSICS2D_PHYSICSSETTINGS2D_H
