/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** AudioSystem header
*/

#ifndef GAME_AUDIOSYSTEM_H
#define GAME_AUDIOSYSTEM_H

#include "library/RessourceManager.h"
#include <string>

namespace Game {

    /**
     * @brief System responsible for sound effects and background music playback.
     */
    class AudioSystem {
    public:
        /**
         * @brief Plays a sound effect by asset name.
         * Note: Feature not available in engine_client:
         * - ResourceManager supports Sound and Music asset types, but engine_client
         *   has no audio playback system (e.g. sf::Sound, sf::Music wrapper).
         */
        static void playSoundEffect(ResourceManager &resources, const std::string &soundName);

        /**
         * @brief Plays background music track by asset name.
         */
        static void playMusic(ResourceManager &resources, const std::string &musicName);
    };

} // namespace Game

#endif // GAME_AUDIOSYSTEM_H
