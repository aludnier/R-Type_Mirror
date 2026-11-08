/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** PlayerLives.cpp
*/

#include "PlayerLives.h"

#include <algorithm>
#include <ranges>

namespace RType {
PlayerLives::PlayerLives(std::uint8_t startLives, float invulnerability) :
    _startLives(startLives), _invulnerability(invulnerability)
{
}

bool PlayerLives::track(std::size_t entity)
{
    return _players.try_emplace(entity, State {_startLives}).second;
}

void PlayerLives::forgetIf(const std::function<bool(std::size_t)> &left)
{
    std::erase_if(_players, [&](const auto &pair) { return left(pair.first); });
}

void PlayerLives::update(float dt)
{
    for (auto &state : _players | std::views::values) {
        state.invulnerable = std::max(state.invulnerable - dt, 0.f);
    }
}

PlayerLives::Hit PlayerLives::hit(std::size_t entity)
{
    auto it = _players.find(entity);

    if (it == _players.end() || it->second.lives == 0 ||
        it->second.invulnerable > 0.f) {
        return Hit::Ignored;
    }
    State &state = it->second;

    --state.lives;
    if (state.lives == 0) {
        return Hit::Out;
    }
    state.invulnerable = _invulnerability;
    return Hit::Respawn;
}

std::uint8_t PlayerLives::lives(std::size_t entity) const
{
    auto it = _players.find(entity);

    return it == _players.end() ? 0 : it->second.lives;
}

std::unordered_map<std::size_t, std::uint8_t> PlayerLives::all() const
{
    std::unordered_map<std::size_t, std::uint8_t> lives;

    for (const auto &[entity, state] : _players) {
        lives.emplace(entity, state.lives);
    }
    return lives;
}
} // namespace RType
