#include <criterion/criterion.h>

#include <tuple>
#include <vector>
#include <stdexcept>

#include "ecs/Component.h"
#include "ecs/Entity.h"
#include "ecs/IndexedZipper.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"
#include "ecs/Zipper.h"

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
}

Test(SparseArrayStoresInsertedValues, insertedComponentsRemainAccessible) {
    RType::SparseArray<Position> array;

    array.insert_at(2, Position{10, 20});
    array.insert_at(5, Position{30, 40});

    cr_assert(array[2].has_value());
    cr_assert(array[5].has_value());
    cr_assert_eq(array[2]->x, 10);
    cr_assert_eq(array[2]->y, 20);
    cr_assert_eq(array[5]->x, 30);
    cr_assert_eq(array[5]->y, 40);
}

Test(SparseArrayEmplacesComponents, emplaceBuildsComponentInPlace) {
    RType::SparseArray<Velocity> array;

    array.emplace(4, 7, 8);

    cr_assert(array[4].has_value());
    cr_assert_eq(array[4]->dx, 7);
    cr_assert_eq(array[4]->dy, 8);
    cr_assert_eq(array.size(), 5u);
}

Test(SparseArrayErasesComponents, eraseRemovesComponentWithoutShrinkingStorage) {
    RType::SparseArray<Health> array;

    array.insert_at(0, Health{99});
    array.insert_at(3, Health{11});

    array.erase(0);

    cr_assert_not(array[0].has_value());
    cr_assert(array[3].has_value());
    cr_assert_eq(array.size(), 4u);
    cr_assert_eq(array.count(), 1u);
}

Test(SparseArrayFindsIndex, getIndexReturnsStoredOptionalPosition) {
    RType::SparseArray<Position> array;

    array.insert_at(9, Position{1, 2});
    auto &slot = array[9];

    cr_assert_eq(array.getIndex(slot), 9u);
}

Test(SparseArrayFindsIndex, getIndexThrowsForUnknownOptional) {
    RType::SparseArray<Position> array;
    std::optional<Position> missing = std::nullopt;

    cr_assert_throw(array.getIndex(missing), std::out_of_range);
}

Test(RegistrySpawnsEntities, uniqueEntityIdsAreProducedInOrder) {
    RType::Registry registry;

    auto first = registry.spawnEntity();
    auto second = registry.spawnEntity();

    cr_assert_neq(first, second);
    cr_assert_eq(static_cast<RType::entity_id_type>(first), 0u);
    cr_assert_eq(static_cast<RType::entity_id_type>(second), 1u);
}

Test(RegistryRegistersComponents, componentStorageIsCreatedAndReturned) {
    RType::Registry registry;

    auto &positions = registry.registerComponent<Position>();
    auto entity = registry.spawnEntity();

    registry.addComponent<Position>(entity, Position{5, 6});

    cr_assert(positions[static_cast<RType::entity_id_type>(entity)].has_value());
    cr_assert_eq(positions[static_cast<RType::entity_id_type>(entity)]->x, 5);
    cr_assert_eq(positions[static_cast<RType::entity_id_type>(entity)]->y, 6);
}

Test(RegistryRemovesComponents, removeComponentClearsTheComponentSlot) {
    RType::Registry registry;
    auto entity = registry.spawnEntity();

    registry.registerComponent<Position>();
    registry.addComponent<Position>(entity, Position{8, 9});
    registry.removeComponent<Position>(entity);

    cr_assert_not(registry.getComponents<Position>()[static_cast<RType::entity_id_type>(entity)].has_value());
}

Test(RegistryRunsSystems, systemsSeeComponentsBeforePendingEntitiesAreRemoved) {
    RType::Registry registry;
    registry.registerComponent<Position>();

    auto entity = registry.spawnEntity();
    registry.addComponent<Position>(entity, Position{12, 13});

    bool systemSawEntity = false;
    registry.addSystem<Position>([&](RType::storage_t<Position> &positions) {
        systemSawEntity = positions[static_cast<RType::entity_id_type>(entity)].has_value();
    });

    registry.killEntity(entity);
    registry.runSystems();

    cr_assert(systemSawEntity);
    cr_assert_not(registry.getComponents<Position>()[static_cast<RType::entity_id_type>(entity)].has_value());
}

Test(RegistryRunsSystems, systemsCanMutateValuesInStorage) {
    RType::Registry registry;
    registry.registerComponent<Position>();

    auto entity = registry.spawnEntity();
    registry.addComponent<Position>(entity, Position{1, 1});

    registry.addSystem<Position>([](RType::storage_t<Position> &positions) {
        auto &component = positions[0];
        cr_assert(component.has_value());
        component->x += 5;
        component->y += 7;
    });

    registry.runSystems();

    cr_assert_eq(registry.getComponents<Position>()[0]->x, 6);
    cr_assert_eq(registry.getComponents<Position>()[0]->y, 8);
}

Test(ZipperIteratesValues, onlyPositionsPresentInAllContainersAreVisited) {
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

    cr_assert_eq(seen.size(), 2u);
    cr_assert_eq(seen[0], std::make_pair(10, 1));
    cr_assert_eq(seen[1], std::make_pair(50, 3));
}

Test(ZipperIteratesValues, shorterContainerLimitsTheIterationRange) {
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

    cr_assert_eq(seen.size(), 1u);
    cr_assert_eq(seen[0], 11);
}

Test(IndexedZipperIncludesIndex, eachYieldContainsTheCurrentEntityIndex) {
    RType::SparseArray<Position> positions;

    positions.emplace(0, 5, 6);
    positions.emplace(2, 7, 8);
    positions.emplace(4, 9, 10);

    RType::IndexedZipper zipper(positions);
    std::vector<std::tuple<std::size_t, int, int>> seen;

    for (auto [index, position] : zipper) {
        seen.emplace_back(index, position.x, position.y);
    }

    cr_assert_eq(seen.size(), 3u);
    cr_assert_eq(seen[0], std::make_tuple(0u, 5, 6));
    cr_assert_eq(seen[1], std::make_tuple(2u, 7, 8));
    cr_assert_eq(seen[2], std::make_tuple(4u, 9, 10));
}

Test(IndexedZipperIncludesIndex, missingValuesAreSkippedBeforeIndexIsReported) {
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
        cr_assert_eq(position.x, 1);
        cr_assert_eq(velocity.dx, 10);
    }

    cr_assert_eq(seen.size(), 1u);
    cr_assert_eq(seen[0], 0u);
}
