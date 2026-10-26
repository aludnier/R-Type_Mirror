/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** AudioSystem implementation
*/

#include "systems/AudioSystem.h"

namespace Game {

void AudioSystem::playSoundEffect(
    const ResourceManager &resources, const std::string &soundName)
{
    (void) resources;
    (void) soundName;
    // Note: Audio playback backend missing in engine_client.
    // resources.getAsset(soundName);
    // sf::Sound sound; sound.play();
}

void AudioSystem::playMusic(
    const ResourceManager &resources, const std::string &musicName)
{
    (void) resources;
    (void) musicName;
    // Note: Music playback backend missing in engine_client.
    // resources.getAsset(musicName);
    // sf::Music music; music.play();
}

} // namespace Game
