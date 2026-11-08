/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Audio source component
*/

#ifndef ENGINE_AUDIOSOURCE_H_
#define ENGINE_AUDIOSOURCE_H_

#include <cstdint>

namespace RType {

struct AudioSource {
    std::uint32_t assetId = 0;
    float volume = 1.f;
    float pitch = 1.f;
    bool loop = false;
    bool playOnSpawn = false;
    bool playing = false;

    bool operator==(const AudioSource &) const = default;
};

} // namespace RType

#endif // ENGINE_AUDIOSOURCE_H_
