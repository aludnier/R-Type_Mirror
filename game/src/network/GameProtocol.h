/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** GameProtocol.h
*/

#ifndef GAME_GAMEPROTOCOL_H
#define GAME_GAMEPROTOCOL_H

#include <algorithm>
#include <cstdint>

#include "components/Action.h"
#include "math/Vector2.h"
#include "network/Protocol.h"
#include "systems/ControllableSystem.h"

/**
 * @file GameProtocol.h
 * @brief Components the R-Type game adds to the engine protocol, shared by
 *        the client and the server. Mirrors docs/protocol.md, Appendix B.
 */
namespace Game::Protocol {
namespace Action {
    constexpr RType::ActionId MoveUp = 0;
    constexpr RType::ActionId MoveDown = 1;
    constexpr RType::ActionId MoveLeft = 2;
    constexpr RType::ActionId MoveRight = 3;
    constexpr RType::ActionId Shoot = 4;
} // namespace Action

constexpr RType::MovementActions MOVEMENT_ACTIONS {
    Action::MoveUp, Action::MoveDown, Action::MoveLeft, Action::MoveRight};

/** @brief ENTITY_KIND component: `kind: u8`, sent in ENTITY_SPAWN. */
constexpr RType::Network::ComponentTypeId ENTITY_KIND = 0x8000;

/** @brief PLAYER_LIVES component: `lives: u8`, in COMPONENT_UPDATE. */
constexpr RType::Network::ComponentTypeId PLAYER_LIVES = 0x8001;

/** @brief Lives of a player when it joins. */
constexpr std::uint8_t PLAYER_START_LIVES = 3;

enum class EntityKind : std::uint8_t {
    Player = 0,
    Enemy = 1,
    PlayerLaser = 2,
    ChargedBeam = 3,
    EnemyBullet = 4,
    /** @brief Enemy that shoots, with more health than a Bydo scout. */
    Gunner = 5,
    /** @brief The boss at the end of each level. */
    Boss = 6,
    /** @brief Pickup dropped by an enemy: gives the double shot. */
    PowerUp = 7,
    /** @brief Enemy that walks on the ground and shoots upwards. */
    Walker = 8,
};

/** @brief Height of the top of the ground, in pixels: touching it costs a
 *         life. */
constexpr float GROUND_TOP = 960.f;
/** @brief The ground scrolls to the left at this speed, in pixels/s. */
constexpr float GROUND_SCROLL_SPEED = 120.f;

/** @brief Size of a player's ship and of its hitbox, in pixels. */
constexpr float PLAYER_WIDTH = 56.f;
constexpr float PLAYER_HEIGHT = 24.f;

/**
 * @return @p position moved inside the play area (1920 x 1080) and above
 *         the ground, for the center of a player's ship.
 */
constexpr RType::Vector2 clampToPlayArea(RType::Vector2 position)
{
    const float halfWidth = PLAYER_WIDTH / 2.f;
    const float halfHeight = PLAYER_HEIGHT / 2.f;

    position.x = std::clamp(position.x, halfWidth, 1920.f - halfWidth);
    position.y = std::clamp(position.y, halfHeight, GROUND_TOP - halfHeight);
    return position;
}

constexpr bool isValidKind(std::uint8_t kind)
{
    return kind <= static_cast<std::uint8_t>(EntityKind::Walker);
}
} // namespace Game::Protocol

#endif // GAME_GAMEPROTOCOL_H
