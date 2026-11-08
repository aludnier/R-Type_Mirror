/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** SFML audio playback manager
*/

#include "audio/AudioManager.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "GameAssetTypes.h"

namespace RType::Game {

AudioManager::AudioManager(ClientAssetManager &assets, std::size_t voices) :
    _assets(assets), _voices(voices)
{
    if (voices == 0)
        throw std::invalid_argument("AudioManager requires at least one voice");
}

AudioManager::~AudioManager()
{
    stopAll();
}

RType::SoundHandle AudioManager::playSoundImpl(
    const std::string &name, float volume, float pitch, bool loop)
{
    const GameSoundAsset *asset;
    try {
        asset = &_assets.load<GameSoundAsset>(name);
    } catch (const std::exception &error) {
        std::cerr << "AudioManager: cannot play sound '" << name
                  << "': " << error.what() << '\n';
        return {};
    }

    std::size_t index = _voices.size();
    for (std::size_t i = 0; i < _voices.size(); ++i) {
        auto &voice = _voices[i];
        if (!voice.sound ||
            voice.sound->getStatus() == sf::SoundSource::Status::Stopped) {
            if (voice.sound)
                voice.sound.reset();
            index = i;
            break;
        }
    }
    if (index == _voices.size()) {
        index = static_cast<std::size_t>(
            std::ranges::min_element(_voices,
                [](const Voice &left, const Voice &right) {
                    return left.startOrder < right.startOrder;
                }) -
            _voices.begin());
    }

    Voice &voice = _voices[index];
    if (voice.sound)
        voice.sound->stop();
    voice.sound.reset();
    ++voice.generation;
    voice.sound.emplace(asset->resource);
    voice.sound->setVolume(effectiveVolume(AudioChannel::Sound, volume));
    voice.sound->setPitch(pitch);
    voice.sound->setLooping(loop);
    voice.sound->play();
    voice.startOrder = _nextStartOrder++;
    voice.baseVolume = std::clamp(volume, 0.f, 1.f);
    voice.assetName = name;
    voice.pausedByAll = false;
    return {static_cast<std::uint32_t>(index), voice.generation};
}

void AudioManager::stopSound(RType::SoundHandle handle)
{
    if (!matches(handle))
        return;
    auto &voice = _voices[handle.index];
    voice.sound->stop();
    voice.sound.reset();
    voice.assetName.clear();
    voice.pausedByAll = false;
}

bool AudioManager::isSoundPlaying(RType::SoundHandle handle) const
{
    if (!matches(handle))
        return false;
    return _voices[handle.index].sound->getStatus() ==
        sf::SoundSource::Status::Playing;
}

void AudioManager::playMusicImpl(
    const std::string &name, bool loop, float volume)
{
    const GameMusicAsset *asset;
    try {
        asset = &_assets.load<GameMusicAsset>(name);
        if (!asset->resource)
            throw std::runtime_error("music asset has no stream");
    } catch (const std::exception &error) {
        std::cerr << "AudioManager: cannot play music '" << name
                  << "': " << error.what() << '\n';
        return;
    }

    stopMusic();
    _music = asset->resource;
    _musicAssetName = name;
    _musicBaseVolume = std::clamp(volume, 0.f, 1.f);
    _music->setLooping(loop);
    _music->setVolume(effectiveVolume(AudioChannel::Music, _musicBaseVolume));
    _music->play();
}

void AudioManager::stopMusic()
{
    if (_music)
        _music->stop();
    _music.reset();
    _musicAssetName.clear();
    _musicPausedByAll = false;
}

void AudioManager::pauseMusic()
{
    if (_music)
        _music->pause();
}

void AudioManager::resumeMusic()
{
    if (_music)
        _music->play();
}

bool AudioManager::isMusicPlaying() const
{
    return _music && _music->getStatus() == sf::SoundSource::Status::Playing;
}

void AudioManager::setVolume(RType::AudioChannel channel, float volume)
{
    _volumes[channelIndex(channel)] = std::clamp(volume, 0.f, 1.f);
    applyVolumes();
}

float AudioManager::getVolume(RType::AudioChannel channel) const
{
    return _volumes[channelIndex(channel)];
}

void AudioManager::pauseAll()
{
    for (auto &voice : _voices) {
        if (voice.sound &&
            voice.sound->getStatus() == sf::SoundSource::Status::Playing) {
            voice.sound->pause();
            voice.pausedByAll = true;
        }
    }
    if (_music && _music->getStatus() == sf::SoundSource::Status::Playing) {
        _music->pause();
        _musicPausedByAll = true;
    }
}

void AudioManager::resumeAll()
{
    for (auto &voice : _voices) {
        if (voice.sound && voice.pausedByAll) {
            voice.sound->play();
            voice.pausedByAll = false;
        }
    }
    if (_music && _musicPausedByAll) {
        _music->play();
        _musicPausedByAll = false;
    }
}

void AudioManager::stopAll()
{
    for (auto &voice : _voices) {
        if (voice.sound)
            voice.sound->stop();
        voice.sound.reset();
        voice.assetName.clear();
        voice.pausedByAll = false;
    }
    stopMusic();
}

void AudioManager::update()
{
    recycleFinishedVoices();
}

void AudioManager::stopAllUsing(const std::string &name)
{
    for (auto &voice : _voices) {
        if (voice.sound && voice.assetName == name) {
            voice.sound->stop();
            voice.sound.reset();
            voice.assetName.clear();
            voice.pausedByAll = false;
        }
    }
    if (_musicAssetName == name)
        stopMusic();
}

std::size_t AudioManager::channelIndex(RType::AudioChannel channel)
{
    switch (channel) {
        case AudioChannel::Master:
            return 0;
        case AudioChannel::Music:
            return 1;
        case AudioChannel::Sound:
            return 2;
    }
    throw std::invalid_argument("AudioManager: invalid audio channel");
}

float AudioManager::effectiveVolume(
    RType::AudioChannel channel, float baseVolume) const
{
    return _volumes[0] * _volumes[channelIndex(channel)] *
        std::clamp(baseVolume, 0.f, 1.f) * 100.f;
}

bool AudioManager::matches(RType::SoundHandle handle) const
{
    return handle.index < _voices.size() &&
        _voices[handle.index].generation == handle.generation &&
        _voices[handle.index].sound.has_value();
}

void AudioManager::applyVolumes()
{
    for (auto &voice : _voices)
        if (voice.sound)
            voice.sound->setVolume(
                effectiveVolume(AudioChannel::Sound, voice.baseVolume));
    if (_music)
        _music->setVolume(
            effectiveVolume(AudioChannel::Music, _musicBaseVolume));
}

void AudioManager::recycleFinishedVoices()
{
    for (auto &voice : _voices) {
        if (voice.sound &&
            voice.sound->getStatus() == sf::SoundSource::Status::Stopped) {
            voice.sound.reset();
            voice.assetName.clear();
            voice.pausedByAll = false;
        }
    }
}

} // namespace RType::Game
