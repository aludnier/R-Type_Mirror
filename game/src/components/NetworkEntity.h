/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** NetworkEntity component header
*/

#ifndef GAME_NETWORKENTITY_H
#define GAME_NETWORKENTITY_H

#include "network/Protocol.h"

namespace Game {

    /**
     * @brief Component associating an ECS entity with a replicated server NetworkId.
     */
    struct NetworkEntity {
        RType::Network::NetworkId networkId = RType::Network::NO_ENTITY;
        RType::Network::Tick lastUpdateTick = 0;
        bool isLocalPlayer = false;

        constexpr NetworkEntity() = default;
        constexpr explicit NetworkEntity(RType::Network::NetworkId id, bool local = false, RType::Network::Tick tick = 0)
            : networkId(id), lastUpdateTick(tick), isLocalPlayer(local) {}

        constexpr bool operator==(const NetworkEntity &other) const = default;
    };

} // namespace Game

#endif // GAME_NETWORKENTITY_H
