/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ShipTiltSystem header
*/

#ifndef GAME_SHIPTILTSYSTEM_H
#define GAME_SHIPTILTSYSTEM_H

#include <array>
#include <cstddef>
#include <unordered_map>

#include "components/Sprite.h"
#include "ecs/Registry.h"
#include "library/InputManager.h"

namespace Game {
/**
 * @brief Tilts the players' ships with their vertical movement: level at
 *        rest, banking up or down while they move. The local player uses
 *        its inputs, the other players the change of their position.
 */
class ShipTiltSystem {
public:
    /** @brief A frame of the ship in the laser_bullet sheet. */
    struct Frame {
        int left;
        int top;
        int width;
        int height;
    };

    /**
     * @brief The ship banking down the most, then down, level, up and up
     *        the most.
     */
    static constexpr std::array<Frame, 5> FRAMES = {{
        {101, 3, 32, 14},
        {134, 3, 32, 13},
        {167, 3, 32, 12},
        {200, 3, 32, 13},
        {233, 2, 32, 14},
    }};
    static constexpr std::size_t LEVEL_FRAME = 2;

    /** @brief Seconds to bank from level to the most. */
    static constexpr float BANK_TIME = 0.2f;

    void update(RType::Registry &registry, const InputManager &input, float dt);

    /** @brief Forgets the ships, for example after a disconnection. */
    void reset();

    /**
     * @return @p tilt moved towards @p target (-1 down, 0 level, 1 up) by
     *         at most dt / BANK_TIME.
     */
    [[nodiscard]] static float approach(float tilt, float target, float dt);

    /** @return The index in FRAMES of a tilt in [-1, 1]. */
    [[nodiscard]] static std::size_t frameIndex(float tilt);

    /** @brief Shows the frame of @p tilt in @p sprite. */
    static void applyFrame(RType::Sprite &sprite, float tilt);

private:
    struct Ship {
        float tilt = 0.f;
        float lastY = 0.f;
        bool seen = false;
    };

    std::unordered_map<std::size_t, Ship> _ships;
};
} // namespace Game

#endif // GAME_SHIPTILTSYSTEM_H
