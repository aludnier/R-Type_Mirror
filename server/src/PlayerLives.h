/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** PlayerLives.h
*/

#ifndef PLAYERLIVES_H
#define PLAYERLIVES_H

#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>

namespace RType {
/**
 * @brief Lives of the players, by entity: a hit takes a life, then the
 *        player is invulnerable for a while; without lives left it is out.
 */
class PlayerLives {
public:
    /** @brief What a hit did to the player. */
    enum class Hit {
        /** @brief Not a player, invulnerable or already out. */
        Ignored,
        /** @brief Lost a life and respawns, invulnerable. */
        Respawn,
        /** @brief Lost its last life. */
        Out,
    };

    /**
     * @param startLives Lives of a player when it joins.
     * @param invulnerability Seconds without taking hits after a respawn.
     */
    PlayerLives(std::uint8_t startLives, float invulnerability);

    /** @return true if @p entity was not tracked yet and now is. */
    bool track(std::size_t entity);

    /** @brief Stops tracking the players for which @p left is true. */
    void forgetIf(const std::function<bool(std::size_t)> &left);

    /** @brief Counts down the invulnerability of every player. */
    void update(float dt);

    Hit hit(std::size_t entity);

    /** @return The lives of @p entity, 0 if it is not tracked. */
    [[nodiscard]] std::uint8_t lives(std::size_t entity) const;

    /** @return Every tracked player and its lives. */
    [[nodiscard]] std::unordered_map<std::size_t, std::uint8_t> all() const;

private:
    struct State {
        std::uint8_t lives;
        float invulnerable = 0.f;
    };

    std::uint8_t _startLives;
    float _invulnerability;
    std::unordered_map<std::size_t, State> _players;
};
} // namespace RType

#endif // PLAYERLIVES_H
