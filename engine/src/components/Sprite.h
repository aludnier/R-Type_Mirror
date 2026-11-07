/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Sprite component
*/

#ifndef SPRITE_H
#define SPRITE_H

#include <cstdint>

namespace RType {
/**
 * @brief 2D Sprite component referencing a registered asset texture.
 */
struct Sprite {
    std::uint32_t assetId = 0;
    int rectLeft = 0;
    int rectTop = 0;
    int rectWidth = 0; // 0 means full texture width
    int rectHeight = 0; // 0 means full texture height
    float originX = 0.5f; // 0.0 to 1.0 origin fraction (0.5 = centered)
    float originY = 0.5f;
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;
    int zIndex = 0;

    constexpr Sprite() = default;

    constexpr explicit Sprite(std::uint32_t assetId, int zIndex = 0) :
        assetId(assetId), zIndex(zIndex)
    {
    }

    constexpr Sprite(std::uint32_t assetId, int rectLeft, int rectTop,
        int rectWidth, int rectHeight, int zIndex = 0) :
        assetId(assetId),
        rectLeft(rectLeft),
        rectTop(rectTop),
        rectWidth(rectWidth),
        rectHeight(rectHeight),
        zIndex(zIndex)
    {
    }

    constexpr bool operator==(const Sprite &o) const = default;
};
} // namespace RType

#endif // SPRITE_H
