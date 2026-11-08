/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ECS audio system
*/

#include "systems/AudioSystem.h"

#include <iostream>
#include <limits>

#include "components/AudioSource.h"
#include "components/MusicPlayer.h"
#include "ecs/SparseArray.h"

namespace RType {

void AudioSystem::update(
    Registry &registry, const AssetManager &assets, IAudioPlayer &audio)
{
    if (registry.isRegistered<AudioSource>()) {
        auto &sources = registry.getComponents<AudioSource>();
        for (auto it = _sources.begin(); it != _sources.end();) {
            const Entity entity = Registry::entityFromIndex(it->first);
            if (!registry.isAlive(entity) || !sources[it->first]) {
                audio.stopSound(it->second.handle);
                it = _sources.erase(it);
            } else {
                ++it;
            }
        }

        for (const Entity entity : registry.entities()) {
            const auto entityId = static_cast<entity_id_type>(entity);
            auto &component = sources[entityId];
            if (!component)
                continue;

            auto [stateIt, inserted] = _sources.try_emplace(entityId);
            auto &state = stateIt->second;
            AudioSource &source = *component;
            if (inserted && source.playOnSpawn)
                source.playing = true;

            if (state.started &&
                (!source.playing || state.assetId != source.assetId)) {
                audio.stopSound(state.handle);
                state.handle = {};
                state.started = false;
            }

            if (source.playing && !state.started) {
                const auto name = assets.getAssetName(source.assetId);
                if (!name) {
                    std::cerr << "AudioSystem: unknown sound asset id "
                              << source.assetId << '\n';
                    source.playing = false;
                    continue;
                }
                state.handle = audio.playSound(
                    *name, source.volume, source.pitch, source.loop);
                state.assetId = source.assetId;
                state.started = state.handle.index !=
                    std::numeric_limits<std::uint32_t>::max();
                if (!state.started)
                    source.playing = false;
            }

            if (state.started && !source.loop &&
                !audio.isSoundPlaying(state.handle)) {
                audio.stopSound(state.handle);
                state.handle = {};
                state.started = false;
                source.playing = false;
            }
        }
    } else {
        for (const auto &[entity, state] : _sources) {
            (void) entity;
            audio.stopSound(state.handle);
        }
        _sources.clear();
    }

    const MusicPlayer *musicPlayer = nullptr;
    if (registry.isRegistered<MusicPlayer>()) {
        auto &musicPlayers = registry.getComponents<MusicPlayer>();
        for (const Entity entity : registry.entities()) {
            auto &component = musicPlayers[static_cast<entity_id_type>(entity)];
            if (component) {
                musicPlayer = &*component;
                break;
            }
        }
    }

    if (!musicPlayer) {
        if (_music) {
            audio.stopMusic();
            _music.reset();
        }
        return;
    }

    const MusicState desired {
        musicPlayer->assetId, musicPlayer->loop, musicPlayer->volume};
    if (_music && _music->assetId == desired.assetId &&
        _music->loop == desired.loop && _music->volume == desired.volume)
        return;

    const auto name = assets.getAssetName(desired.assetId);
    if (!name) {
        std::cerr << "AudioSystem: unknown music asset id " << desired.assetId
                  << '\n';
        audio.stopMusic();
        _music = desired;
        return;
    }
    audio.playMusic(*name, desired.loop, desired.volume);
    _music = desired;
}

} // namespace RType
