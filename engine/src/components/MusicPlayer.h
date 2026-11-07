/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Music player component
*/

#ifndef ENGINE_MUSICPLAYER_H_
#define ENGINE_MUSICPLAYER_H_

#include <cstdint>

namespace RType {

struct MusicPlayer {
    std::uint32_t assetId = 0;
    bool loop = true;
    float volume = 1.f;

    bool operator==(const MusicPlayer &) const = default;
};

} // namespace RType

#endif // ENGINE_MUSICPLAYER_H_