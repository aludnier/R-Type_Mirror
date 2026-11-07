#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <string>

#include "audio/IAudioPlayer.h"
#include "scripting/AudioBindings.h"

namespace {

class MockAudioPlayer final : public RType::IAudioPlayer {
public:
    RType::SoundHandle playSound(const std::string &name, float volume,
        float pitch, bool loop) override
    {
        soundName = name;
        soundVolume = volume;
        soundPitch = pitch;
        soundLoop = loop;
        ++playSoundCalls;
        return soundHandle;
    }

    void stopSound(RType::SoundHandle handle) override
    {
        stoppedSound = handle;
        ++stopSoundCalls;
    }

    [[nodiscard]] bool isSoundPlaying(RType::SoundHandle) const override
    {
        return false;
    }

    void playMusic(const std::string &name, bool loop, float volume) override
    {
        musicName = name;
        musicLoop = loop;
        musicVolume = volume;
        ++playMusicCalls;
    }

    void stopMusic() override
    {
        ++stopMusicCalls;
    }

    void pauseMusic() override
    {
        ++pauseMusicCalls;
    }

    void resumeMusic() override
    {
        ++resumeMusicCalls;
    }

    [[nodiscard]] bool isMusicPlaying() const override
    {
        return false;
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
        ++pauseAllCalls;
    }

    void resumeAll() override
    {
        ++resumeAllCalls;
    }

    void stopAll() override
    {
        ++stopAllCalls;
    }

    RType::SoundHandle soundHandle {17, 3};
    RType::SoundHandle stoppedSound;
    std::string soundName;
    float soundVolume = 0.f;
    float soundPitch = 0.f;
    bool soundLoop = false;
    std::string musicName;
    float musicVolume = 0.f;
    bool musicLoop = false;
    std::array<float, 3> volumes {1.f, 1.f, 1.f};
    int playSoundCalls = 0;
    int stopSoundCalls = 0;
    int playMusicCalls = 0;
    int stopMusicCalls = 0;
    int pauseMusicCalls = 0;
    int resumeMusicCalls = 0;
    int pauseAllCalls = 0;
    int resumeAllCalls = 0;
    int stopAllCalls = 0;
};

} // namespace

TEST_CASE("Lua audio bindings forward calls with defaults and handles",
    "[scripting][audio]")
{
    sol::state lua;
    lua.open_libraries(sol::lib::base);
    MockAudioPlayer audio;
    RType::Scripting::bindAudio(lua, audio);

    const auto result = lua.safe_script(R"(
        local sound = audio.play_sound("sfx_laser")
        sound_handle_index = sound.index
        sound_handle_generation = sound.generation
        audio.stop_sound(sound)
        audio.play_sound("sfx_hit", 0.25, 1.5)
        audio.play_music("bgm_stage1")
        audio.play_music("bgm_boss", false, 0.4)
        audio.pause_music()
        audio.resume_music()
        audio.stop_music()
        audio.set_master_volume(0.7)
        audio.set_music_volume(0.5)
        audio.set_sound_volume(0.3)
        audio.pause_all()
        audio.resume_all()
        audio.stop_all()
    )");
    REQUIRE(result.valid());

    CHECK(lua["sound_handle_index"].get<std::uint32_t>() == 17);
    CHECK(lua["sound_handle_generation"].get<std::uint32_t>() == 3);
    CHECK(audio.playSoundCalls == 2);
    CHECK(audio.stopSoundCalls == 1);
    CHECK(audio.stoppedSound.index == audio.soundHandle.index);
    CHECK(audio.soundName == "sfx_hit");
    CHECK(audio.soundVolume == 0.25f);
    CHECK(audio.soundPitch == 1.5f);
    CHECK_FALSE(audio.soundLoop);
    CHECK(audio.playMusicCalls == 2);
    CHECK(audio.musicName == "bgm_boss");
    CHECK_FALSE(audio.musicLoop);
    CHECK(audio.musicVolume == 0.4f);
    CHECK(audio.stopMusicCalls == 1);
    CHECK(audio.pauseMusicCalls == 1);
    CHECK(audio.resumeMusicCalls == 1);
    CHECK(audio.volumes[static_cast<std::size_t>(RType::AudioChannel::Master)] ==
        0.7f);
    CHECK(audio.volumes[static_cast<std::size_t>(RType::AudioChannel::Music)] ==
        0.5f);
    CHECK(audio.volumes[static_cast<std::size_t>(RType::AudioChannel::Sound)] ==
        0.3f);
    CHECK(audio.pauseAllCalls == 1);
    CHECK(audio.resumeAllCalls == 1);
    CHECK(audio.stopAllCalls == 1);
}