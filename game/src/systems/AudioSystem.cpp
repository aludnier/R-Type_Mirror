/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** AudioSystem implementation
*/

#include "systems/AudioSystem.h"

namespace Game {

void AudioSystem::playSoundEffect(
    RType::Game::AudioManager &audio, const std::string &soundName)
{
    audio.playSound(soundName);
}

void AudioSystem::playMusic(
    RType::Game::AudioManager &audio, const std::string &musicName)
{
    audio.playMusic(musicName);
}

} // namespace Game
