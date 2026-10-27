#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <stdexcept>
#include <tuple>
#include <vector>

#include "components/Controllable.h"
#include "components/Health.h"
#include "components/Hitbox.h"
#include "components/Transform2D.h"
#include "ecs/Component.h"
#include "ecs/Entity.h"
#include "ecs/IndexedZipper.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"
#include "ecs/Zipper.h"
#include "library/InputManager.h"
#include "systems/ControllableSystem.h"

namespace {
struct Position {
    int x;
    int y;
};

struct Velocity {
    int dx;
    int dy;
};

struct Health {
    int hp;
};
} // namespace

TEST_CASE("SparseArray stores inserted values", "[ecs][sparse-array]")
{
    RType::SparseArray<Position> array;

    array.insert_at(2, Position {10, 20});
    array.insert_at(5, Position {30, 40});

    REQUIRE(array[2].has_value());
    REQUIRE(array[5].has_value());
    REQUIRE(array[2]->x == 10);
    REQUIRE(array[2]->y == 20);
    REQUIRE(array[5]->x == 30);
    REQUIRE(array[5]->y == 40);
}

TEST_CASE("SparseArray emplaces components", "[ecs][sparse-array]")
{
    RType::SparseArray<Velocity> array;

    array.emplace(4, 7, 8);

    REQUIRE(array[4].has_value());
    REQUIRE(array[4]->dx == 7);
    REQUIRE(array[4]->dy == 8);
    REQUIRE(array.size() == 5u);
}

TEST_CASE("SparseArray erases components", "[ecs][sparse-array]")
{
    RType::SparseArray<Health> array;

    array.insert_at(0, Health {99});
    array.insert_at(3, Health {11});

    array.erase(0);

    REQUIRE_FALSE(array[0].has_value());
    REQUIRE(array[3].has_value());
    REQUIRE(array[3]->hp == 11);
    REQUIRE(array.size() == 4u);
    REQUIRE(array.count() == 1u);
}

TEST_CASE("SparseArray getIndex returns stored optional position",
    "[ecs][sparse-array]")
{
    RType::SparseArray<Position> array;

    array.insert_at(9, Position {1, 2});
    auto &slot = array[9];

    REQUIRE(array.getIndex(slot) == 9u);
}

TEST_CASE(
    "SparseArray getIndex throws for unknown optional", "[ecs][sparse-array]")
{
    RType::SparseArray<Position> array;
    std::optional<Position> missing = std::nullopt;

    REQUIRE_THROWS_AS(array.getIndex(missing), std::out_of_range);
}

TEST_CASE("Registry spawns entities", "[ecs][registry]")
{
    RType::Registry registry;

    auto first = registry.spawnEntity();
    auto second = registry.spawnEntity();

    REQUIRE(first != second);
    REQUIRE(static_cast<RType::entity_id_type>(first) == 0u);
    REQUIRE(static_cast<RType::entity_id_type>(second) == 1u);
}

TEST_CASE("Registry registers components", "[ecs][registry]")
{
    RType::Registry registry;

    auto &positions = registry.registerComponent<Position>();
    auto entity = registry.spawnEntity();

    registry.addComponent<Position>(entity, Position {5, 6});

    REQUIRE(positions[static_cast<RType::entity_id_type>(entity)].has_value());
    REQUIRE(positions[static_cast<RType::entity_id_type>(entity)]->x == 5);
    REQUIRE(positions[static_cast<RType::entity_id_type>(entity)]->y == 6);
}

TEST_CASE("Registry removes components", "[ecs][registry]")
{
    RType::Registry registry;
    auto entity = registry.spawnEntity();

    registry.registerComponent<Position>();
    registry.addComponent<Position>(entity, Position {8, 9});
    registry.removeComponent<Position>(entity);

    REQUIRE_FALSE(
        registry
            .getComponents<Position>()[static_cast<RType::entity_id_type>(
                entity)]
            .has_value());
}

TEST_CASE("Registry systems see components before pending entities are removed",
    "[ecs][registry]")
{
    RType::Registry registry;
    registry.registerComponent<Position>();

    auto entity = registry.spawnEntity();
    registry.addComponent<Position>(entity, Position {12, 13});

    bool systemSawEntity = false;
    registry.addSystem<Position>([&](RType::storage_t<Position> &positions) {
        systemSawEntity =
            positions[static_cast<RType::entity_id_type>(entity)].has_value();
    });

    registry.killEntity(entity);
    registry.runSystems();

    REQUIRE(systemSawEntity);
    REQUIRE_FALSE(
        registry
            .getComponents<Position>()[static_cast<RType::entity_id_type>(
                entity)]
            .has_value());
}

TEST_CASE("Registry systems can mutate values in storage", "[ecs][registry]")
{
    RType::Registry registry;
    registry.registerComponent<Position>();

    auto entity = registry.spawnEntity();
    registry.addComponent<Position>(entity, Position {1, 1});

    registry.addSystem<Position>([](RType::storage_t<Position> &positions) {
        auto &component = positions[0];
        REQUIRE(component.has_value());
        component->x += 5;
        component->y += 7;
    });

    registry.runSystems();

    REQUIRE(registry.getComponents<Position>()[0]->x == 6);
    REQUIRE(registry.getComponents<Position>()[0]->y == 8);
}

TEST_CASE("Controllable system moves entities from their action mask",
    "[systems][controllable]")
{
    RType::Registry registry;
    registry.registerComponent<RType::Transform2D>();
    registry.registerComponent<RType::Controllable>();

    const auto entity = registry.spawnEntity();
    registry.addComponent<RType::Transform2D>(entity, RType::Transform2D {});
    registry.addComponent<RType::Controllable>(entity,
        RType::Controllable {static_cast<std::uint8_t>(
            (1u << static_cast<std::uint8_t>(Action::MoveRight)) |
            (1u << static_cast<std::uint8_t>(Action::MoveDown)))});
    RType::addControllableSystem(registry, 10.f, 0.5f);

    registry.runSystems();

    const auto &transform = registry.getComponents<
        RType::Transform2D>()[static_cast<RType::entity_id_type>(entity)];
    REQUIRE(transform->position == RType::Vector2 {5.f, 5.f});
}

TEST_CASE("Controllable system cancels opposing actions and ignores shooting",
    "[systems][controllable]")
{
    RType::Registry registry;
    registry.registerComponent<RType::Transform2D>();
    registry.registerComponent<RType::Controllable>();

    const auto entity = registry.spawnEntity();
    registry.addComponent<RType::Transform2D>(entity, RType::Transform2D {});
    registry.addComponent<RType::Controllable>(entity,
        RType::Controllable {static_cast<std::uint8_t>(
            (1u << static_cast<std::uint8_t>(Action::MoveLeft)) |
            (1u << static_cast<std::uint8_t>(Action::MoveRight)) |
            (1u << static_cast<std::uint8_t>(Action::Shoot)))});
    RType::addControllableSystem(registry, 10.f, 1.f);

    registry.runSystems();

    const auto &transform = registry.getComponents<
        RType::Transform2D>()[static_cast<RType::entity_id_type>(entity)];
    REQUIRE(transform->position == RType::Vector2 {});
}

TEST_CASE("Zipper iterates values present in all containers", "[ecs][zipper]")
{
    RType::SparseArray<Position> positions;
    RType::SparseArray<Velocity> velocities;

    positions.emplace(0, 10, 20);
    positions.emplace(1, 30, 40);
    positions.emplace(2, 50, 60);

    velocities.emplace(0, 1, 2);
    velocities.emplace(2, 3, 4);

    RType::Zipper zipper(positions, velocities);
    std::vector<std::pair<int, int>> seen;

    for (auto [position, velocity] : zipper) {
        seen.emplace_back(position.x, velocity.dx);
    }

    REQUIRE(seen.size() == 2u);
    REQUIRE(seen[0] == std::make_pair(10, 1));
    REQUIRE(seen[1] == std::make_pair(50, 3));
}

TEST_CASE("Zipper over storages with no common value is empty", "[ecs][zipper]")
{
    RType::SparseArray<Position> positions;
    RType::SparseArray<Velocity> velocities;

    positions.emplace(0, 1, 2);
    velocities.emplace(1, 10, 20);

    RType::Zipper zipper(positions, velocities);

    REQUIRE(zipper.begin() == zipper.end());
}

TEST_CASE("Zipper shorter container limits iteration range", "[ecs][zipper]")
{
    RType::SparseArray<Position> positions;
    RType::SparseArray<Velocity> velocities;

    positions.emplace(0, 1, 2);
    positions.emplace(1, 3, 4);
    positions.emplace(2, 5, 6);
    positions.emplace(3, 7, 8);

    velocities.emplace(0, 10, 20);

    RType::Zipper zipper(positions, velocities);
    std::vector<int> seen;

    for (auto [position, velocity] : zipper) {
        seen.push_back(position.x + velocity.dx);
    }

    REQUIRE(seen.size() == 1u);
    REQUIRE(seen[0] == 11);
}

TEST_CASE("Zipper skips leading unset optionals", "[ecs][zipper]")
{
    RType::SparseArray<Position> positions;
    RType::SparseArray<Velocity> velocities;

    positions.emplace(2, 50, 60);
    positions.emplace(4, 70, 80);

    velocities.emplace(0, 1, 2);
    velocities.emplace(2, 3, 4);
    velocities.emplace(4, 5, 6);

    RType::Zipper zipper(positions, velocities);
    std::vector<std::pair<int, int>> seen;

    for (auto [position, velocity] : zipper) {
        seen.emplace_back(position.x, velocity.dx);
    }

    REQUIRE(seen.size() == 2u);
    REQUIRE(seen[0] == std::make_pair(50, 3));
    REQUIRE(seen[1] == std::make_pair(70, 5));
}

TEST_CASE(
    "IndexedZipper includes current entity index", "[ecs][indexed-zipper]")
{
    RType::SparseArray<Position> positions;

    positions.emplace(0, 5, 6);
    positions.emplace(2, 7, 8);
    positions.emplace(4, 9, 10);

    RType::IndexedZipper zipper(positions);
    std::vector<std::tuple<std::size_t, int, int>> seen;

    for (auto [index, position] : zipper) {
        seen.emplace_back(index, position.x, position.y);
    }

    REQUIRE(seen.size() == 3u);
    REQUIRE((seen[0] == std::make_tuple(0u, 5, 6)));
    REQUIRE((seen[1] == std::make_tuple(2u, 7, 8)));
    REQUIRE((seen[2] == std::make_tuple(4u, 9, 10)));
}

TEST_CASE("IndexedZipper skips missing values before reporting index",
    "[ecs][indexed-zipper]")
{
    RType::SparseArray<Position> positions;
    RType::SparseArray<Velocity> velocities;

    positions.emplace(0, 1, 2);
    positions.emplace(1, 3, 4);

    velocities.emplace(0, 10, 20);
    velocities.emplace(3, 30, 40);

    RType::IndexedZipper zipper(positions, velocities);
    std::vector<std::size_t> seen;

    for (auto [index, position, velocity] : zipper) {
        seen.push_back(index);
        REQUIRE(position.x == 1);
        REQUIRE(velocity.dx == 10);
    }

    REQUIRE(seen.size() == 1u);
    REQUIRE(seen[0] == 0u);
}

TEST_CASE("Health component behaves correctly", "[ecs][components][health]")
{
    RType::Health health(10, 20);

    REQUIRE(health.current == 10);
    REQUIRE(health.max == 20);
    REQUIRE_FALSE(health.isDead());

    health.takeDamage(5);
    REQUIRE(health.current == 5);
    REQUIRE_FALSE(health.isDead());

    health.takeDamage(10);
    REQUIRE(health.current == 0);
    REQUIRE(health.isDead());
}

TEST_CASE(
    "Hitbox component initializes dimensions", "[ecs][components][hitbox]")
{
    RType::Hitbox defaultBox;
    REQUIRE(defaultBox.size.x == 32.0f);
    REQUIRE(defaultBox.size.y == 32.0f);

    RType::Hitbox customBox(64.0f, 48.0f);
    REQUIRE(customBox.size.x == 64.0f);
    REQUIRE(customBox.size.y == 48.0f);
}
