/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** RTypeServer.cpp
*/

#include "RTypeServer.h"

#include <cmath>
#include <cstdint>
#include <unordered_set>

#include "components/Action.h"
#include "components/Controllable.h"
#include "components/Health.h"
#include "components/Hitbox.h"
#include "components/Transform2D.h"
#include "ecs/SparseArray.h"
#include "network/ByteWriter.h"

namespace RType {
    namespace {
        using Game::Protocol::EntityKind;

        // Play area, in pixels: the client window (docs/protocol.md,
        // Appendix B).
        constexpr float SCREEN_WIDTH = 1920.f;
        constexpr float SCREEN_HEIGHT = 1080.f;
        /** @brief Margin past the screen edges before an entity is removed. */
        constexpr float OUT_OF_BOUNDS_MARGIN = 100.f;

        constexpr float CHARGE_TIME = 1.5f;
        constexpr float LASER_COOLDOWN = 0.15f;

        constexpr float ENEMY_SPAWN_INTERVAL = 1.5f;
        constexpr float ENEMY_SPEED = 150.f;
        constexpr float ENEMY_WAVE_AMPLITUDE = 60.f;
        constexpr float ENEMY_WAVE_FREQUENCY = 2.f;
        constexpr float ENEMY_SHOOT_INTERVAL = 2.f;
        constexpr int ENEMY_HEALTH = 2;

        /** @brief A projectile moving in a straight line. */
        struct Projectile {
            Vector2 velocity;
            int damage = 1;
            bool fromPlayer = true;
            /** @brief Goes through the enemies it hits (charged beam). */
            bool piercing = false;
        };

        /** @brief A Bydo flying left on a sine wave. */
        struct Enemy {
            float baseY = 0.f;
            float elapsed = 0.f;
            float shootTimer = 0.f;
        };

        struct ProjectileStats {
            Vector2 velocity;
            Vector2 hitbox;
            int damage;
            bool piercing;
        };

        ProjectileStats statsOf(EntityKind kind)
        {
            switch (kind) {
                case EntityKind::ChargedBeam:
                    return {{900.f, 0.f}, {48.f, 24.f}, 5, true};
                case EntityKind::EnemyBullet:
                    return {{-450.f, 0.f}, {12.f, 12.f}, 1, false};
                default:
                    return {{700.f, 0.f}, {16.f, 8.f}, 1, false};
            }
        }

        bool isShooting(std::uint8_t actionMask)
        {
            return (actionMask & (1u << static_cast<unsigned>(Action::Shoot))) !=
                0;
        }

        Network::ComponentData encodeKind(EntityKind kind)
        {
            Network::ByteWriter writer;

            writer.write(static_cast<std::uint8_t>(kind));
            return {Game::Protocol::ENTITY_KIND, writer.release()};
        }

        /** @brief PHYSICS component (docs/protocol.md, section 6.3). */
        Network::ComponentData encodePhysics(
            const Vector2 &velocity, const Vector2 &hitbox)
        {
            Network::ByteWriter writer;

            writer.writeFloat(velocity.x);
            writer.writeFloat(velocity.y);
            writer.writeFloat(hitbox.x);
            writer.writeFloat(hitbox.y);
            return {static_cast<Network::ComponentTypeId>(
                        Network::EngineComponentType::Physics),
                writer.release()};
        }

        bool overlap(const Vector2 &posA, const Vector2 &sizeA,
            const Vector2 &posB, const Vector2 &sizeB)
        {
            return std::abs(posA.x - posB.x) * 2.f < sizeA.x + sizeB.x &&
                std::abs(posA.y - posB.y) * 2.f < sizeA.y + sizeB.y;
        }

        template<class C>
        bool has(const SparseArray<C> &storage, std::size_t entity)
        {
            return entity < storage.size() && storage[entity].has_value();
        }
    } // namespace

    RTypeServer::RTypeServer(ServerConfig config) : GameServer(config)
    {
        Registry &reg = registry();

        reg.registerComponent<Projectile>();
        reg.registerComponent<Enemy>();
        reg.registerComponent<Health>();
        reg.registerComponent<Hitbox>();
        // Runs after the engine's systems: players have moved this tick.
        reg.addSystem<Transform2D>([this](auto &) { step(); });
    }

    void RTypeServer::step()
    {
        updateWeapons();
        spawnEnemies();
        updateEnemies();
        moveProjectiles();
        resolveHits();
        cleanupOutOfBounds();
        flushSpawns();
    }

    void RTypeServer::updateWeapons()
    {
        const auto &transforms = registry().getComponents<Transform2D>();
        const auto &controllables = registry().getComponents<Controllable>();
        float dt = tickDelta();

        // Players that left: their entity has no Controllable anymore.
        std::erase_if(_weapons,
            [&](const auto &pair) { return !has(controllables, pair.first); });
        for (std::size_t i = 0; i < controllables.size(); ++i) {
            if (!controllables[i] || !has(transforms, i)) {
                continue;
            }
            Weapon &weapon = _weapons[i];
            bool shooting = isShooting(controllables[i]->actionMask);
            Vector2 position = transforms[i]->position;

            weapon.cooldown -= dt;
            if (shooting) {
                weapon.chargeTime += dt;
            } else if (weapon.wasShooting) {
                // Fires when the button is released, like the client shows
                // it: a full charge gives the wave cannon.
                if (weapon.chargeTime >= CHARGE_TIME) {
                    _pendingSpawns.push_back({EntityKind::ChargedBeam,
                        {position.x + 35.f, position.y}});
                } else if (weapon.cooldown <= 0.f) {
                    _pendingSpawns.push_back({EntityKind::PlayerLaser,
                        {position.x + 25.f, position.y}});
                    weapon.cooldown = LASER_COOLDOWN;
                }
                weapon.chargeTime = 0.f;
            }
            weapon.wasShooting = shooting;
        }
    }

    void RTypeServer::spawnEnemies()
    {
        _enemySpawnTimer += tickDelta();
        if (_enemySpawnTimer < ENEMY_SPAWN_INTERVAL) {
            return;
        }
        _enemySpawnTimer = 0.f;
        std::uniform_real_distribution<float> y(
            ENEMY_WAVE_AMPLITUDE + 40.f,
            SCREEN_HEIGHT - ENEMY_WAVE_AMPLITUDE - 40.f);

        _pendingSpawns.push_back(
            {EntityKind::Enemy, {SCREEN_WIDTH + 40.f, y(_random)}});
    }

    void RTypeServer::updateEnemies()
    {
        auto &transforms = registry().getComponents<Transform2D>();
        auto &enemies = registry().getComponents<Enemy>();
        float dt = tickDelta();

        for (std::size_t i = 0; i < enemies.size(); ++i) {
            if (!enemies[i] || !has(transforms, i)) {
                continue;
            }
            Enemy &enemy = *enemies[i];
            Vector2 &position = transforms[i]->position;

            enemy.elapsed += dt;
            position.x -= ENEMY_SPEED * dt;
            position.y = enemy.baseY +
                std::sin(enemy.elapsed * ENEMY_WAVE_FREQUENCY) *
                    ENEMY_WAVE_AMPLITUDE;
            enemy.shootTimer += dt;
            if (enemy.shootTimer >= ENEMY_SHOOT_INTERVAL &&
                position.x < SCREEN_WIDTH) {
                enemy.shootTimer = 0.f;
                _pendingSpawns.push_back({EntityKind::EnemyBullet,
                    {position.x - 20.f, position.y}});
            }
        }
    }

    void RTypeServer::moveProjectiles()
    {
        auto &transforms = registry().getComponents<Transform2D>();
        const auto &projectiles = registry().getComponents<Projectile>();
        float dt = tickDelta();

        for (std::size_t i = 0; i < projectiles.size(); ++i) {
            if (projectiles[i] && has(transforms, i)) {
                transforms[i]->position += projectiles[i]->velocity * dt;
            }
        }
    }

    void RTypeServer::resolveHits()
    {
        const auto &transforms = registry().getComponents<Transform2D>();
        const auto &hitboxes = registry().getComponents<Hitbox>();
        const auto &projectiles = registry().getComponents<Projectile>();
        const auto &enemies = registry().getComponents<Enemy>();
        const auto &controllables = registry().getComponents<Controllable>();
        auto &healths = registry().getComponents<Health>();
        // Killed entities keep their components until the end of the tick.
        std::unordered_set<std::size_t> destroyed;

        for (std::size_t p = 0; p < projectiles.size(); ++p) {
            if (!projectiles[p] || !has(transforms, p) || !has(hitboxes, p)) {
                continue;
            }
            const Projectile &projectile = *projectiles[p];

            for (std::size_t t = 0; t < transforms.size(); ++t) {
                if (destroyed.contains(p)) {
                    break;
                }
                if (t == p || destroyed.contains(t) || !transforms[t]) {
                    continue;
                }
                bool isTarget = projectile.fromPlayer ? has(enemies, t)
                                                      : has(controllables, t);
                // Players have no Hitbox component: the size of the ship.
                Vector2 targetSize =
                    has(hitboxes, t) ? hitboxes[t]->size : Vector2 {40.f, 20.f};

                if (!isTarget ||
                    !overlap(transforms[p]->position, hitboxes[p]->size,
                        transforms[t]->position, targetSize)) {
                    continue;
                }
                if (!projectile.piercing) {
                    destroyed.insert(p);
                    destroy(p);
                }
                // Players do not lose lives yet: the bullet just vanishes.
                if (projectile.fromPlayer && has(healths, t)) {
                    healths[t]->takeDamage(projectile.damage);
                    if (healths[t]->isDead()) {
                        destroyed.insert(t);
                        destroy(t);
                    }
                }
            }
        }
    }

    void RTypeServer::cleanupOutOfBounds()
    {
        const auto &transforms = registry().getComponents<Transform2D>();
        const auto &projectiles = registry().getComponents<Projectile>();
        const auto &enemies = registry().getComponents<Enemy>();

        for (std::size_t i = 0; i < transforms.size(); ++i) {
            if (!transforms[i] || (!has(projectiles, i) && !has(enemies, i))) {
                continue;
            }
            const Vector2 &pos = transforms[i]->position;

            if (pos.x < -OUT_OF_BOUNDS_MARGIN ||
                pos.x > SCREEN_WIDTH + OUT_OF_BOUNDS_MARGIN ||
                pos.y < -OUT_OF_BOUNDS_MARGIN ||
                pos.y > SCREEN_HEIGHT + OUT_OF_BOUNDS_MARGIN) {
                destroy(i);
            }
        }
    }

    void RTypeServer::flushSpawns()
    {
        // Spawned after the loops: adding components while iterating over a
        // storage could reallocate it.
        for (const auto &request : _pendingSpawns) {
            spawn(request);
        }
        _pendingSpawns.clear();
    }

    void RTypeServer::spawn(const SpawnRequest &request)
    {
        Registry &reg = registry();
        Entity entity = reg.spawnEntity();

        reg.addComponent(entity, Transform2D {request.position});
        if (request.kind == EntityKind::Enemy) {
            Vector2 size {40.f, 30.f};

            reg.addComponent(entity, Enemy {request.position.y, 0.f, 0.f});
            reg.addComponent(entity, Health {ENEMY_HEALTH});
            reg.addComponent(entity, Hitbox {size});
            replicateEntity(entity,
                {encodeKind(request.kind), encodePhysics({0.f, 0.f}, size)});
            return;
        }
        ProjectileStats stats = statsOf(request.kind);

        reg.addComponent(entity,
            Projectile {stats.velocity, stats.damage,
                request.kind != EntityKind::EnemyBullet, stats.piercing});
        reg.addComponent(entity, Hitbox {stats.hitbox});
        // Straight line: the clients move it themselves from its PHYSICS.
        replicateEntity(entity,
            {encodeKind(request.kind),
                encodePhysics(stats.velocity, stats.hitbox)},
            true);
    }

    void RTypeServer::destroy(std::size_t entity)
    {
        despawnEntity(Registry::entityFromIndex(entity));
    }
} // namespace RType
