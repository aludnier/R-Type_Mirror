/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** GameProtocol.h
*/

#ifndef GAME_GAMEPROTOCOL_H
#define GAME_GAMEPROTOCOL_H

#include <cstdint>

#include "network/Protocol.h"

/**
 * @file GameProtocol.h
 * @brief Components the R-Type game adds to the engine protocol, shared by
 *        the client and the server. Mirrors docs/protocol.md, Appendix B.
 */
namespace Game::Protocol {
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
