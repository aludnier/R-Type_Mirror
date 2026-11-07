/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** SFML audio playback manager
*/

#ifndef ENGINE_AUDIOMANAGER_H_
#define ENGINE_AUDIOMANAGER_H_

#include <SFML/Audio.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "audio/IAudioPlayer.h"
#include "library/ClientAssetManager.h"

namespace RType::Game {

class AudioManager final : public RType::IAudioPlayer {
public:
    explicit AudioManager(ClientAssetManager &assets, std::size_t voices = 32);
    ~AudioManager() override;

    RType::SoundHandle playSound(const std::string &name, float volume = 1.f,
        float pitch = 1.f, bool loop = false) override;
    void stopSound(RType::SoundHandle handle) override;
    [[nodiscard]] bool isSoundPlaying(RType::SoundHandle handle) const override;

    void playMusic(const std::string &name, bool loop = true,
        float volume = 1.f) override;
    void stopMusic() override;
    void pauseMusic() override;
    void resumeMusic() override;
    [[nodiscard]] bool isMusicPlaying() const override;

    void setVolume(RType::AudioChannel channel, float volume) override;
    [[nodiscard]] float getVolume(RType::AudioChannel channel) const override;

    void pauseAll() override;
    void resumeAll() override;
    void stopAll() override;

    void update();
    void stopAllUsing(const std::string &name);

private:
    struct Voice {
        std::optional<sf::Sound> sound;
        std::uint32_t generation = 0;
        std::uint64_t startOrder = 0;
        float baseVolume = 1.f;
        std::string assetName;
        bool pausedByAll = false;
    };

    [[nodiscard]] static std::size_t channelIndex(RType::AudioChannel channel);
    [[nodiscard]] float effectiveVolume(
        RType::AudioChannel channel, float baseVolume) const;
    [[nodiscard]] bool matches(RType::SoundHandle handle) const;
    void applyVolumes();
    void recycleFinishedVoices();

    ClientAssetManager &_assets;
    std::vector<Voice> _voices;
    std::array<float, 3> _volumes {1.f, 1.f, 1.f};
    std::shared_ptr<sf::Music> _music;
    std::string _musicAssetName;
    std::uint64_t _nextStartOrder = 0;
    float _musicBaseVolume = 1.f;
    bool _musicPausedByAll = false;
};

} // namespace RType::Game

#endif // ENGINE_AUDIOMANAGER_H_