/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ECS audio system
*/

#ifndef ENGINE_AUDIOSYSTEM_H_
#define ENGINE_AUDIOSYSTEM_H_

#include <optional>
#include <unordered_map>

#include "audio/IAudioPlayer.h"
#include "ecs/Registry.h"
#include "library/AssetManager.h"

namespace RType {

class AudioSystem {
public:
    void update(
        Registry &registry, const AssetManager &assets, IAudioPlayer &audio);

private:
    struct SourceState {
        SoundHandle handle;
        std::uint32_t assetId = 0;
        bool started = false;
    };

    struct MusicState {
        std::uint32_t assetId;
        bool loop;
        float volume;
    };

    std::unordered_map<entity_id_type, SourceState> _sources;
    std::optional<MusicState> _music;
};

} // namespace RType

#endif // ENGINE_AUDIOSYSTEM_H_
