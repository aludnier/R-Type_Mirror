/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** RTypeServer.h
*/

#ifndef RTYPESERVER_H
#define RTYPESERVER_H

#include <cstddef>
#include <random>
#include <unordered_map>
#include <vector>

#include "ecs/Entity.h"
#include "math/Vector2.h"
#include "GameProtocol.h"
#include "server/GameServer.h"

namespace RType {
    /**
     * @brief The R-Type game on top of the engine server: the server decides
     *        when a player shoots, spawns the Bydos and resolves the hits, so
     *        that every client sees the same projectiles and enemies.
     */
    class RTypeServer : public GameServer {
    public:
        explicit RTypeServer(ServerConfig config);

    private:
        /** @brief A projectile or an enemy to spawn once the loop is over. */
        struct SpawnRequest {
            Game::Protocol::EntityKind kind;
            Vector2 position;
        };

        /** @brief Shooting state of a player, kept between ticks. */
        struct Weapon {
            bool wasShooting = false;
            float chargeTime = 0.f;
            float cooldown = 0.f;
        };

        void step();
        void updateWeapons();
        void spawnEnemies();
        void updateEnemies();
        void moveProjectiles();
        void resolveHits();
        void cleanupOutOfBounds();

        /** @brief Spawns and replicates the requests queued this tick. */
        void flushSpawns();
        void spawn(const SpawnRequest &request);
        void destroy(std::size_t entity);

        std::vector<SpawnRequest> _pendingSpawns;
        std::unordered_map<std::size_t, Weapon> _weapons;
        float _enemySpawnTimer = 0.f;
        std::mt19937 _random {std::random_device {}()};
    };
} // namespace RType

#endif // RTYPESERVER_H
