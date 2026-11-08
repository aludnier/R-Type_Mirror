/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** SFML-free audio playback interface
*/

#ifndef ENGINE_IAUDIOPLAYER_H_
#define ENGINE_IAUDIOPLAYER_H_

#include <cstdint>
#include <limits>
#include <string>

namespace RType {

struct SoundHandle {
    std::uint32_t index = std::numeric_limits<std::uint32_t>::max();
    std::uint32_t generation = 0;
};

enum class AudioChannel {
    Master,
    Music,
    Sound
};

class IAudioPlayer {
public:
    virtual ~IAudioPlayer() = default;

    SoundHandle playSound(const std::string &name, float volume = 1.f,
        float pitch = 1.f, bool loop = false)
    {
        return playSoundImpl(name, volume, pitch, loop);
    }

    virtual void stopSound(SoundHandle handle) = 0;
    [[nodiscard]] virtual bool isSoundPlaying(SoundHandle handle) const = 0;

    void playMusic(
        const std::string &name, bool loop = true, float volume = 1.f)
    {
        playMusicImpl(name, loop, volume);
    }

    virtual void stopMusic() = 0;
    virtual void pauseMusic() = 0;
    virtual void resumeMusic() = 0;
    [[nodiscard]] virtual bool isMusicPlaying() const = 0;

    virtual void setVolume(AudioChannel channel, float volume) = 0;
    [[nodiscard]] virtual float getVolume(AudioChannel channel) const = 0;

    virtual void pauseAll() = 0;
    virtual void resumeAll() = 0;
    virtual void stopAll() = 0;

private:
    virtual SoundHandle playSoundImpl(
        const std::string &name, float volume, float pitch, bool loop) = 0;
    virtual void playMusicImpl(
        const std::string &name, bool loop, float volume) = 0;
};

} // namespace RType

#endif // ENGINE_IAUDIOPLAYER_H_