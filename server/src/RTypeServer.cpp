/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** RTypeServer.cpp
*/

#include "RTypeServer.h"

#include <cmath>
#include <cstdint>

#include "Enemy.h"
#include "Physics2DBindings.h"
#include "RTypeScripting.h"
#include "components/BoxCollider2D.h"
#include "components/CollisionEvents2D.h"
#include "components/Controllable.h"
#include "components/Health.h"
#include "components/Hitbox.h"
#include "components/Rigidbody2D.h"
#include "components/Transform2D.h"
#include "ecs/SparseArray.h"
#include "network/ByteWriter.h"
#include "scripting/ScriptComponent.h"

namespace RType {
namespace {
    using Game::Protocol::EntityKind;
    namespace Action = Game::Protocol::Action;

    // Play area, in pixels: the client window (docs/protocol.md,
    // Appendix B).
    constexpr float SCREEN_WIDTH = 1920.f;
    constexpr float SCREEN_HEIGHT = 1080.f;

    constexpr float CHARGE_TIME = 1.5f;
    constexpr float LASER_COOLDOWN = 0.15f;

    constexpr int ENEMY_HEALTH = 2;
    /** @brief Entity script of every Bydo, in the script directory. */
    constexpr const char *ENEMY_SCRIPT = "bydo.lua";

    /** @brief A projectile moving in a straight line. */
    struct Projectile {
        Vector2 velocity;
        int damage = 1;
        bool fromPlayer = true;
        /** @brief Goes through the enemies it hits (charged beam). */
        bool piercing = false;
    };

    struct ProjectileStats {
        Vector2 velocity;
        Vector2 hitbox;
        int damage {};
        bool piercing {};
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

    bool isShooting(ActionMask actionMask)
    {
        return (actionMask & (ActionMask {1} << Action::Shoot)) != 0;
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

    template<class C>
    bool has(const SparseArray<C> &storage, std::size_t entity)
    {
        return entity < storage.size() && storage[entity].has_value();
    }
} // namespace

RTypeServer::RTypeServer(const ServerConfig &config) :
    GameServer(config), _scripts(registry())
{
    setup(ENGINE_MODULE_DIR "/../config/scripts.json");
}

RTypeServer::RTypeServer(const Configuration &configuration) :
    GameServer(configuration), _scripts(registry())
{
    // Next to the game configuration.
    setup(configuration.path().parent_path() / "scripts.json");
}

void RTypeServer::setup(const std::filesystem::path &scriptsConfig)
{
    Registry &reg = registry();

    reg.registerComponent<Projectile>();
    reg.registerComponent<Enemy>();
    reg.registerComponent<Health>();
    reg.registerComponent<Hitbox>();
    // Runs after the engine's systems: players have moved this tick.
    reg.addSystem<Transform2D>([this](auto &) { step(); });

    // An entity killed by a script also disappears on the clients.
    _scripts.setKillHandler([this](Entity entity) {
        despawnEntity(entity);
        // Not replicated: despawnEntity() did not kill it.
        if (!registry().isPendingKill(entity)) {
            registry().killEntity(entity);
        }
    });
    // Its components are registered only when the module is loaded.
    if (isModuleLoaded(Physics2D::MODULE_NAME)) {
        Physics2D::bindScripting(_scripts);
    }
    // Spawned at the end of the tick, like the projectiles.
    bindRTypeScripting(_scripts,
        {SCREEN_WIDTH, SCREEN_HEIGHT},
        [this](EntityKind kind, Vector2 position) {
            _pendingSpawns.push_back({kind, position});
        });
    // Projectile is in an anonymous namespace, so bind it here.
    _scripts.lua().new_usertype<Projectile>("Projectile",
        sol::constructors<Projectile>(),
        "velocity",
        &Projectile::velocity,
        "damage",
        &Projectile::damage,
        "is_player_bullet",
        &Projectile::fromPlayer,
        "is_charged_beam",
        &Projectile::piercing);
    _scripts.bindComponent<Projectile>("Projectile");
    _scripts.loadConfigFile(scriptsConfig);
}

void RTypeServer::step()
{
    updateWeapons();
    runScripts();
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
        auto &[wasShooting, chargeTime, cooldown] = _weapons[i];
        bool shooting = isShooting(controllables[i]->actionMask);
        Vector2 position = transforms[i]->position;

        cooldown -= dt;
        if (shooting) {
            chargeTime += dt;
        } else if (wasShooting) {
            // Fires when the button is released, like the client shows
            // it: a full charge gives the wave cannon.
            if (chargeTime >= CHARGE_TIME) {
                _pendingSpawns.push_back(
                    {EntityKind::ChargedBeam, {position.x + 35.f, position.y}});
            } else if (cooldown <= 0.f) {
                _pendingSpawns.push_back(
                    {EntityKind::PlayerLaser, {position.x + 25.f, position.y}});
                cooldown = LASER_COOLDOWN;
            }
            chargeTime = 0.f;
        }
        wasShooting = shooting;
    }
}

void RTypeServer::runScripts()
{
    _scripts.setDeltaTime(tickDelta());
    // A script error is printed by the engine and only disables that
    // script: the game goes on.
    _scripts.update(registry().getComponents<ScriptComponent>());
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
        reg.addComponent(entity, RType::Physics2D::Rigidbody2D {});
        reg.addComponent(entity,
            RType::Physics2D::BoxCollider2D {size, {}, true, 1, 0xFFFFFFFFu});
        reg.addComponent(entity, RType::Physics2D::CollisionEvents2D {});
        reg.addComponent(entity, ScriptComponent {{ENEMY_SCRIPT}});
        replicateEntity(entity,
            {encodeKind(request.kind), encodePhysics({0.f, 0.f}, size)});
        return;
    }
    auto [velocity, hitbox, damage, piercing] = statsOf(request.kind);

    reg.addComponent(entity,
        Projectile {velocity,
            damage,
            request.kind != EntityKind::EnemyBullet,
            piercing});
    reg.addComponent(entity, Hitbox {hitbox});
    {
        RType::Physics2D::Rigidbody2D body;
        body.velocity = velocity;
        body.mass = 1.f;
        body.drag = 0.f;
        body.gravityScale = 0.f;
        body.isKinematic = false;
        body.isStatic = false;
        reg.addComponent<Physics2D::Rigidbody2D>(entity, std::move(body));
    }
    reg.addComponent(entity,
        RType::Physics2D::BoxCollider2D {hitbox, {}, true, 1, 0xFFFFFFFFu});
    reg.addComponent(entity, RType::Physics2D::CollisionEvents2D {});
    // Straight line: the clients move it themselves from its PHYSICS.
    replicateEntity(entity,
        {encodeKind(request.kind), encodePhysics(velocity, hitbox)},
        true);
}

void RTypeServer::destroy(std::size_t entity)
{
    despawnEntity(Registry::entityFromIndex(entity));
}
} // namespace RType
