/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Health component header
*/

#ifndef HEALTH_H
#define HEALTH_H

namespace RType {

    /**
     * @brief Entity health component.
     */
    struct Health {
        int current = 1;
        int max = 1;

        constexpr Health() = default;
        constexpr explicit Health(int health) : current(health), max(health) {}
        constexpr Health(int current, int max) : current(current), max(max) {}

        [[nodiscard]] constexpr bool isDead() const {
            return current <= 0;
        }

        constexpr void takeDamage(int amount) {
            current -= amount;
            if (current < 0) {
                current = 0;
            }
        }

        constexpr bool operator==(const Health &o) const = default;
    };

} // namespace RType

namespace Game {
    using Health = RType::Health;
}

#endif // HEALTH_H
