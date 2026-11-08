/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** AudioSystem header
*/

#ifndef GAME_AUDIOSYSTEM_H
#define GAME_AUDIOSYSTEM_H

#include <string>

#include "audio/AudioManager.h"

namespace Game {

/**
 * @brief System responsible for sound effects and background music playback.
 */
class AudioSystem {
public:
    static void playSoundEffect(
        RType::Game::AudioManager &audio, const std::string &soundName);

    /**
     * @brief Plays background music track by asset name.
     */
    static void playMusic(
        RType::Game::AudioManager &audio, const std::string &musicName);
};

} // namespace Game

#endif // GAME_AUDIOSYSTEM_H
