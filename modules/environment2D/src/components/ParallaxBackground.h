/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ParallaxBackground component header
*/

#ifndef ENVIRONMENT2D_PARALLAXBACKGROUND_H
#define ENVIRONMENT2D_PARALLAXBACKGROUND_H

namespace RType::Environment {

    /**
     * @brief Parallax background component for scrolling space layers.
     */
    struct ParallaxBackground {
        float scrollSpeed = 0.0f;
        float resetX = -1920.0f;
        float startX = 1920.0f;

        constexpr ParallaxBackground() = default;
        constexpr explicit ParallaxBackground(float speed, float resetX = -1920.0f, float startX = 1920.0f)
            : scrollSpeed(speed), resetX(resetX), startX(startX) {}

        constexpr bool operator==(const ParallaxBackground &o) const = default;
    };

} // namespace RType::Environment

#endif // ENVIRONMENT2D_PARALLAXBACKGROUND_H
