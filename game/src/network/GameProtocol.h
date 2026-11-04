/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** GameProtocol.h
*/

#ifndef GAME_GAMEPROTOCOL_H
#define GAME_GAMEPROTOCOL_H

#include <cstdint>

#include "components/Action.h"
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

enum class EntityKind : std::uint8_t {
    Player = 0,
    Enemy = 1,
    PlayerLaser = 2,
    ChargedBeam = 3,
    EnemyBullet = 4,
};

constexpr bool isValidKind(std::uint8_t kind)
{
    return kind <= static_cast<std::uint8_t>(EntityKind::EnemyBullet);
}
} // namespace Game::Protocol

#endif // GAME_GAMEPROTOCOL_H
