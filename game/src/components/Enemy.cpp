/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Enemy component implementation
*/

#include "components/Enemy.h"
#include <utility>

namespace Game {

Enemy::Enemy(std::string type, float interval, int score, float initY) :
    enemyType(std::move(type)),
    shootInterval(interval),
    scoreValue(score),
    initialY(initY)
{
}

} // namespace Game
