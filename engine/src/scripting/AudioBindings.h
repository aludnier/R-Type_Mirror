/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Lua bindings for audio playback
*/

#ifndef AUDIOBINDINGS_H_
#define AUDIOBINDINGS_H_

#include <map>
#include <string>

#include <sol/sol.hpp>

#include "audio/IAudioPlayer.h"

namespace RType::Scripting {

using AudioFunctionFactory = sol::object (*)(sol::state &, IAudioPlayer &);

namespace AudioBindingFunctions {
    sol::object playSound(sol::state &lua, IAudioPlayer &audio);
    sol::object stopSound(sol::state &lua, IAudioPlayer &audio);
    sol::object playMusic(sol::state &lua, IAudioPlayer &audio);
    sol::object stopMusic(sol::state &lua, IAudioPlayer &audio);
    sol::object pauseMusic(sol::state &lua, IAudioPlayer &audio);
    sol::object resumeMusic(sol::state &lua, IAudioPlayer &audio);
    sol::object setMasterVolume(sol::state &lua, IAudioPlayer &audio);
    sol::object setMusicVolume(sol::state &lua, IAudioPlayer &audio);
    sol::object setSoundVolume(sol::state &lua, IAudioPlayer &audio);
    sol::object pauseAll(sol::state &lua, IAudioPlayer &audio);
    sol::object resumeAll(sol::state &lua, IAudioPlayer &audio);
    sol::object stopAll(sol::state &lua, IAudioPlayer &audio);
} // namespace AudioBindingFunctions

inline const std::map<std::string, AudioFunctionFactory> audioFunctions {
    {"play_sound", AudioBindingFunctions::playSound},
    {"stop_sound", AudioBindingFunctions::stopSound},
    {"play_music", AudioBindingFunctions::playMusic},
    {"stop_music", AudioBindingFunctions::stopMusic},
    {"pause_music", AudioBindingFunctions::pauseMusic},
    {"resume_music", AudioBindingFunctions::resumeMusic},
    {"set_master_volume", AudioBindingFunctions::setMasterVolume},
    {"set_music_volume", AudioBindingFunctions::setMusicVolume},
    {"set_sound_volume", AudioBindingFunctions::setSoundVolume},
    {"pause_all", AudioBindingFunctions::pauseAll},
    {"resume_all", AudioBindingFunctions::resumeAll},
    {"stop_all", AudioBindingFunctions::stopAll},
};

void bindAudio(sol::state &lua, IAudioPlayer &audio);

} // namespace RType::Scripting

#endif // AUDIOBINDINGS_H_
