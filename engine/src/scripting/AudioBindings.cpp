/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Lua bindings for audio playback
*/

#include "scripting/AudioBindings.h"

#include <utility>

sol::object RType::Scripting::AudioBindingFunctions::playSound(
    sol::state &lua, IAudioPlayer &audio)
{
    return sol::make_object(lua,
        [&audio](const std::string &name,
            sol::optional<float>
                volume,
            sol::optional<float>
                pitch) {
            return audio.playSound(
                name, volume.value_or(1.f), pitch.value_or(1.f));
        });
}

sol::object RType::Scripting::AudioBindingFunctions::stopSound(
    sol::state &lua, IAudioPlayer &audio)
{
    return sol::make_object(
        lua, [&audio](SoundHandle handle) { audio.stopSound(handle); });
}

sol::object RType::Scripting::AudioBindingFunctions::playMusic(
    sol::state &lua, IAudioPlayer &audio)
{
    return sol::make_object(lua,
        [&audio](const std::string &name,
            sol::optional<bool>
                loop,
            sol::optional<float>
                volume) {
            audio.playMusic(name, loop.value_or(true), volume.value_or(1.f));
        });
}

sol::object RType::Scripting::AudioBindingFunctions::stopMusic(
    sol::state &lua, IAudioPlayer &audio)
{
    return sol::make_object(lua, [&audio] { audio.stopMusic(); });
}

sol::object RType::Scripting::AudioBindingFunctions::pauseMusic(
    sol::state &lua, IAudioPlayer &audio)
{
    return sol::make_object(lua, [&audio] { audio.pauseMusic(); });
}

sol::object RType::Scripting::AudioBindingFunctions::resumeMusic(
    sol::state &lua, IAudioPlayer &audio)
{
    return sol::make_object(lua, [&audio] { audio.resumeMusic(); });
}

sol::object RType::Scripting::AudioBindingFunctions::setMasterVolume(
    sol::state &lua, IAudioPlayer &audio)
{
    return sol::make_object(lua, [&audio](float volume) {
        audio.setVolume(AudioChannel::Master, volume);
    });
}

sol::object RType::Scripting::AudioBindingFunctions::setMusicVolume(
    sol::state &lua, IAudioPlayer &audio)
{
    return sol::make_object(lua, [&audio](float volume) {
        audio.setVolume(AudioChannel::Music, volume);
    });
}

sol::object RType::Scripting::AudioBindingFunctions::setSoundVolume(
    sol::state &lua, IAudioPlayer &audio)
{
    return sol::make_object(lua, [&audio](float volume) {
        audio.setVolume(AudioChannel::Sound, volume);
    });
}

sol::object RType::Scripting::AudioBindingFunctions::pauseAll(
    sol::state &lua, IAudioPlayer &audio)
{
    return sol::make_object(lua, [&audio] { audio.pauseAll(); });
}

sol::object RType::Scripting::AudioBindingFunctions::resumeAll(
    sol::state &lua, IAudioPlayer &audio)
{
    return sol::make_object(lua, [&audio] { audio.resumeAll(); });
}

sol::object RType::Scripting::AudioBindingFunctions::stopAll(
    sol::state &lua, IAudioPlayer &audio)
{
    return sol::make_object(lua, [&audio] { audio.stopAll(); });
}

void RType::Scripting::bindAudio(sol::state &lua, IAudioPlayer &audio)
{
    lua.new_usertype<SoundHandle>("SoundHandle",
        "index",
        &SoundHandle::index,
        "generation",
        &SoundHandle::generation);

    sol::table bindings = lua.create_table();
    for (const auto &[name, factory] : audioFunctions)
        bindings[name] = factory(lua, audio);
    lua["audio"] = std::move(bindings);
}
