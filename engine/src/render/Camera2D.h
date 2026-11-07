/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Camera2D.h
*/

#ifndef CAMERA2D_H_
#define CAMERA2D_H_

#include "math/Vector2.h"

namespace RType {
/** @brief What part of the world is visible. One world unit is one pixel at
 * zoom 1, and y goes down. */
struct Camera2D {
    Vector2 center {0.f, 0.f};
    float zoom = 1.f;
};
} // namespace RType

#endif /* !CAMERA2D_H_ */
