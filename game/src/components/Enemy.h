/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Enemy component header
*/

#ifndef GAME_ENEMY_H
#define GAME_ENEMY_H

#include <string>

namespace Game {

    /**
     * @brief Enemy component for Bydo-inspired enemy ships.
     */
    struct Enemy {
        std::string enemyType = "BydoScout";
        float shootTimer = 0.0f;
        float shootInterval = 2.0f;
        int scoreValue = 100;
        float waveFrequency = 3.0f;
        float waveAmplitude = 120.0f;
        float initialY = 0.0f;
        float elapsedTime = 0.0f;

        Enemy() = default;
        Enemy(std::string type, float interval, int score, float initY);
    };

} // namespace Game

#endif // GAME_ENEMY_H
