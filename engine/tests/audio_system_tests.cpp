#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "audio/IAudioPlayer.h"
#include "components/AudioSource.h"
#include "components/MusicPlayer.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"
#include "library/AssetManager.h"
#include "systems/AudioSystem.h"

namespace {

class MockAudioPlayer final : public RType::IAudioPlayer {
public:
    struct SoundCall {
        RType::SoundHandle handle;
        std::string name;
        float volume;
        float pitch;
        bool loop;
    };

    struct MusicCall {
        std::string name;
        bool loop;
        float volume;
    };

    void stopSound(RType::SoundHandle handle) override
    {
        ++stopSoundCalls;
        if (handle.index < soundPlaying.size())
            soundPlaying[handle.index] = false;
    }

    [[nodiscard]] bool isSoundPlaying(RType::SoundHandle handle) const override
    {
        return handle.index < soundPlaying.size() && soundPlaying[handle.index];
    }

    void stopMusic() override
    {
        ++stopMusicCalls;
        musicPlaying = false;
    }

    void pauseMusic() override
    {
        musicPlaying = false;
    }

    void resumeMusic() override
    {
        musicPlaying = true;
    }

    [[nodiscard]] bool isMusicPlaying() const override
    {
        return musicPlaying;
    }

    void setVolume(RType::AudioChannel channel, float volume) override
    {
        volumes[static_cast<std::size_t>(channel)] = volume;
    }

    [[nodiscard]] float getVolume(RType::AudioChannel channel) const override
    {
        return volumes[static_cast<std::size_t>(channel)];
    }

    void pauseAll() override
    {
        musicPlaying = false;
    }

    void resumeAll() override
    {
        musicPlaying = true;
    }

    void stopAll() override
    {
        std::ranges::fill(soundPlaying, false);
        musicPlaying = false;
    }

    void finishSound(RType::SoundHandle handle)
    {
        soundPlaying[handle.index] = false;
    }

    std::vector<SoundCall> sounds;
    std::vector<MusicCall> music;
    std::vector<bool> soundPlaying;
    std::array<float, 3> volumes {1.f, 1.f, 1.f};
    std::size_t stopSoundCalls = 0;
    std::size_t stopMusicCalls = 0;
    bool musicPlaying = false;

private:
    RType::SoundHandle playSoundImpl(
        const std::string &name, float volume, float pitch, bool loop) override
    {
        const RType::SoundHandle handle {
            static_cast<std::uint32_t>(sounds.size()), 1};
        sounds.push_back({handle, name, volume, pitch, loop});
        soundPlaying.push_back(true);
        return handle;
    }

    void playMusicImpl(
        const std::string &name, bool loop, float volume) override
    {
        music.push_back({name, loop, volume});
        musicPlaying = true;
    }
};

} // namespace

TEST_CASE("AudioSystem follows AudioSource lifecycle", "[audio][ecs]")
{
    RType::Registry registry;
    registry.registerComponent<RType::AudioSource>();
    AssetManager assets;
    const auto soundId = assets.addAsset(
        "sfx_laser", "assets/audio/laser.wav", AssetManager::AssetType::Sound);
    MockAudioPlayer audio;
    RType::AudioSystem system;
    const auto entity = registry.spawnEntity();
    registry.addComponent(entity,
        RType::AudioSource {
            .assetId = soundId,
            .volume = 0.6f,
            .pitch = 1.25f,
            .loop = false,
            .playOnSpawn = true,
            .playing = false,
        });

    system.update(registry, assets, audio);
    auto &source = registry.getComponents<RType::AudioSource>()[entity].value();
    REQUIRE(source.playing);
    REQUIRE(audio.sounds.size() == 1);
    CHECK(audio.sounds[0].name == "sfx_laser");
    CHECK(audio.sounds[0].volume == 0.6f);
    CHECK(audio.sounds[0].pitch == 1.25f);
    CHECK_FALSE(audio.sounds[0].loop);

    audio.finishSound(audio.sounds[0].handle);
    system.update(registry, assets, audio);
    CHECK_FALSE(source.playing);
    CHECK(audio.stopSoundCalls == 1);

    source.playing = true;
    system.update(registry, assets, audio);
    REQUIRE(audio.sounds.size() == 2);
    registry.removeComponent<RType::AudioSource>(entity);
    system.update(registry, assets, audio);
    CHECK(audio.stopSoundCalls == 2);
}

TEST_CASE("AudioSystem stops sources when their entities are destroyed",
    "[audio][ecs]")
{
    RType::Registry registry;
    registry.registerComponent<RType::AudioSource>();
    AssetManager assets;
    const auto soundId = assets.addAsset(
        "sfx_laser", "assets/audio/laser.wav", AssetManager::AssetType::Sound);
    MockAudioPlayer audio;
    RType::AudioSystem system;
    const auto entity = registry.spawnEntity();
    registry.addComponent(entity,
        RType::AudioSource {.assetId = soundId, .loop = true, .playing = true});

    system.update(registry, assets, audio);
    REQUIRE(audio.sounds.size() == 1);
    registry.killEntity(entity);
    registry.killAllEntities();
    system.update(registry, assets, audio);
    CHECK(audio.stopSoundCalls == 1);
}

TEST_CASE(
    "AudioSystem drives the first MusicPlayer as level music", "[audio][ecs]")
{
    RType::Registry registry;
    registry.registerComponent<RType::MusicPlayer>();
    AssetManager assets;
    const auto firstId = assets.addAsset("bgm_stage1",
        "assets/audio/stage1.ogg",
        AssetManager::AssetType::Music);
    const auto secondId = assets.addAsset(
        "bgm_boss", "assets/audio/boss.ogg", AssetManager::AssetType::Music);
    MockAudioPlayer audio;
    RType::AudioSystem system;
    const auto entity = registry.spawnEntity();
    registry.addComponent(entity,
        RType::MusicPlayer {.assetId = firstId, .loop = true, .volume = 0.4f});

    system.update(registry, assets, audio);
    system.update(registry, assets, audio);
    REQUIRE(audio.music.size() == 1);
    CHECK(audio.music[0].name == "bgm_stage1");
    CHECK(audio.music[0].volume == 0.4f);

    registry.getComponents<RType::MusicPlayer>()[entity]->assetId = secondId;
    system.update(registry, assets, audio);
    REQUIRE(audio.music.size() == 2);
    CHECK(audio.music[1].name == "bgm_boss");

    registry.removeComponent<RType::MusicPlayer>(entity);
    system.update(registry, assets, audio);
    CHECK(audio.stopMusicCalls == 1);
}
