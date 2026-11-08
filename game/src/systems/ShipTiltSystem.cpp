/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ShipTiltSystem implementation
*/

#include "systems/ShipTiltSystem.h"

#include <algorithm>
#include <cmath>

#include "components/NetworkEntity.h"
#include "components/Player.h"
#include "components/Transform2D.h"
#include "ecs/IndexedZipper.h"
#include "ecs/SparseArray.h"
#include "network/GameProtocol.h"

namespace Game {
namespace {
    /** @brief Smaller moves of a remote ship are noise, not a direction. */
    constexpr float MIN_SPEED = 30.f;
} // namespace

void ShipTiltSystem::update(
    RType::Registry &registry, const InputManager &input, float dt)
{
    auto &players = registry.getComponents<Player>();
    auto &sprites = registry.getComponents<RType::Sprite>();
    auto &transforms = registry.getComponents<RType::Transform2D>();
    auto &networkEntities = registry.getComponents<NetworkEntity>();

    std::erase_if(_ships, [&](const auto &pair) {
        return pair.first >= players.size() || !players[pair.first];
    });
    for (auto [index, player, sprite, transform, network] :
        RType::IndexedZipper(players, sprites, transforms, networkEntities)) {
        (void) player;
        Ship &ship = _ships[index];
        float target = 0.f;

        if (network.isLocalPlayer) {
            target =
                (input.isActionDown(Protocol::Action::MoveUp) ? 1.f : 0.f) -
                (input.isActionDown(Protocol::Action::MoveDown) ? 1.f : 0.f);
        } else if (ship.seen && dt > 0.f) {
            // y grows downwards: moving up makes it smaller.
            const float speed = (ship.lastY - transform.position.y) / dt;
            if (std::abs(speed) >= MIN_SPEED)
                target = speed > 0.f ? 1.f : -1.f;
        }
        ship.lastY = transform.position.y;
        ship.seen = true;
        ship.tilt = approach(ship.tilt, target, dt);
        applyFrame(sprite, ship.tilt);
    }
}

void ShipTiltSystem::reset()
{
    _ships.clear();
}

float ShipTiltSystem::approach(float tilt, float target, float dt)
{
    const float step = dt / BANK_TIME;

    if (tilt < target)
        return std::min(tilt + step, target);
    return std::max(tilt - step, target);
}

std::size_t ShipTiltSystem::frameIndex(float tilt)
{
    const float clamped = std::clamp(tilt, -1.f, 1.f);
    const auto offset = static_cast<long>(std::lround(clamped * 2.f));

    return static_cast<std::size_t>(static_cast<long>(LEVEL_FRAME) + offset);
}

void ShipTiltSystem::applyFrame(RType::Sprite &sprite, float tilt)
{
    const Frame &frame = FRAMES.at(frameIndex(tilt));

    sprite.rectLeft = frame.left;
    sprite.rectTop = frame.top;
    sprite.rectWidth = frame.width;
    sprite.rectHeight = frame.height;
}
} // namespace Game
