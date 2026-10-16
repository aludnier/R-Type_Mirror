/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** PhysicsSettings2D.cpp
*/

#include "PhysicsSettings2D.h"

namespace RType::Physics2D {
    const PhysicsSettings2D &settingsOf(SparseArray<PhysicsSettings2D> &storage) {
        static const PhysicsSettings2D defaults{};
        for (auto &value: storage)
            if (value.has_value())
                return *value;
        return defaults;
    }
}
