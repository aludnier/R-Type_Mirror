/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ForcePod component header
*/

#ifndef GAME_FORCEPOD_H
#define GAME_FORCEPOD_H

#include <cstddef>

namespace Game {

/**
 * @brief R-Type Force Pod / Satellite attachment component.
 */
struct ForcePod {
    size_t ownerId = 0;
    float offsetX = 50.0f;
    float offsetY = 0.0f;
    int level = 1;

    ForcePod() = default;
    explicit ForcePod(size_t ownerId, float offsetX = 50.0f,
        float offsetY = 0.0f, int level = 1);
};

} // namespace Game

#endif // GAME_FORCEPOD_H
