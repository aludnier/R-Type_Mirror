/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ForcePod component implementation
*/

#include "components/ForcePod.h"

namespace Game {

    ForcePod::ForcePod(size_t ownerId, float offsetX, float offsetY, int level)
        : ownerId(ownerId), offsetX(offsetX), offsetY(offsetY), level(level)
    {
    }

} // namespace Game
