/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Rigidbody2D.h
*/

#ifndef PHYSICS2D_RIGIDBODY2D_H
    #define PHYSICS2D_RIGIDBODY2D_H

#include "math/Vector2.h"

namespace RType::Physics2D {
    class Rigidbody2D {
    public:
        Vector2 velocity{};
        float mass = 1.f;
        float drag = 0.f;
        float gravityScale = 1.f;
        bool isKinematic = false;
        bool isStatic = false;

        void addForce(Vector2 force) { _force += force; }
        void addImpulse(Vector2 impulse) { if (mass > 0.f) velocity += impulse / mass; }

        [[nodiscard]] Vector2 force() const { return _force; }
        void clearForce() { _force = {}; }

    private:
        Vector2 _force{};
    };
}

#endif // PHYSICS2D_RIGIDBODY2D_H
