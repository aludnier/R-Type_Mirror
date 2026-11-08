/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** RTypeServer.h
*/

#ifndef RTYPESERVER_H
#define RTYPESERVER_H

#include <cstddef>
#include <filesystem>
#include <unordered_map>
#include <vector>

#include "GameProtocol.h"
#include "components/Rigidbody2D.h"
#include "math/Vector2.h"
#include "scripting/ScriptEngine.h"
#include "server/GameServer.h"

namespace RType {
/**
 * @brief The R-Type game on top of the engine server: the server decides
 *        when a player shoots, spawns the Bydos and resolves the hits, so
 *        that every client sees the same projectiles and enemies.
 *
 * The enemy waves and the Bydo behaviour are Lua scripts
 * (server/scripts), listed in config/scripts.json.
 */
class RTypeServer : public GameServer {
public:
    explicit RTypeServer(const ServerConfig &config);
    explicit RTypeServer(const Configuration &configuration);

private:
    /** @brief A projectile or an enemy to spawn once the loop is over. */
    struct SpawnRequest {
        Game::Protocol::EntityKind kind {};
        Vector2 position;
    };

    /** @brief Shooting state of a player, kept between ticks. */
    struct Weapon {
        bool wasShooting = false;
        float chargeTime = 0.f;
        float cooldown = 0.f;
    };

    /**
     * @brief Registers the game components and systems, and loads the
     *        scripts listed in @p scriptsConfig.
     * @throws std::runtime_error If a script cannot be loaded.
     */
    void setup(const std::filesystem::path &scriptsConfig);

    void step();
    void updateWeapons();
    /** @brief Runs the game scripts, then the script of each entity. */
    void runScripts();

    /** @brief Spawns and replicates the requests queued this tick. */
    void flushSpawns();
    void spawn(const SpawnRequest &request);
    void destroy(std::size_t entity);

    std::vector<SpawnRequest> _pendingSpawns;
    std::unordered_map<std::size_t, Weapon> _weapons;
    // Member of the derived class: destroyed before the registry.
    ScriptEngine _scripts;
};
} // namespace RType

#endif // RTYPESERVER_H
