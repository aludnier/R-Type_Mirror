#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "audio/IAudioPlayer.h"
#include "systems/NetworkSyncSystem.h"

namespace {

class MockAudioPlayer final : public RType::IAudioPlayer {
public:
    void stopSound(RType::SoundHandle) override
    {
    }

    [[nodiscard]] bool isSoundPlaying(RType::SoundHandle) const override
    {
        return false;
    }

    void stopMusic() override
    {
    }

    void pauseMusic() override
    {
    }

    void resumeMusic() override
    {
    }

    [[nodiscard]] bool isMusicPlaying() const override
    {
        return false;
    }

    void setVolume(RType::AudioChannel, float) override
    {
    }

    [[nodiscard]] float getVolume(RType::AudioChannel) const override
    {
        return 1.f;
    }

    void pauseAll() override
    {
    }

    void resumeAll() override
    {
    }

    void stopAll() override
    {
    }

    std::string lastName;
    float lastVolume = 0.f;
    float lastPitch = 0.f;
    bool lastLoop = false;
    int playCalls = 0;

private:
    RType::SoundHandle playSoundImpl(
        const std::string &name, float volume, float pitch, bool loop) override
    {
        lastName = name;
        lastVolume = volume;
        lastPitch = pitch;
        lastLoop = loop;
        ++playCalls;
        return {0, 1};
    }

    void playMusicImpl(const std::string &, bool, float) override
    {
    }
};

} // namespace

TEST_CASE("NetworkSyncSystem forwards PLAY_SOUND when audio is available",
    "[network][audio]")
{
    Game::NetworkSyncSystem sync;
    RType::Registry registry;
    AssetManager assets;
    MockAudioPlayer audio;
    std::vector<Game::NetworkClient::ServerMessage> messages {
        RType::Network::PlaySoundMsgPacket {
            .assetName = "sfx_explosion", .volume = 0.35f, .pitch = 1.4f}};

    sync.processServerMessages(
        messages, registry, assets, RType::Network::NO_ENTITY, &audio);

    CHECK(audio.playCalls == 1);
    CHECK(audio.lastName == "sfx_explosion");
    CHECK(audio.lastVolume == 0.35f);
    CHECK(audio.lastPitch == 1.4f);
    CHECK_FALSE(audio.lastLoop);
}

TEST_CASE("NetworkSyncSystem ignores PLAY_SOUND when audio is unavailable",
    "[network][audio]")
{
    Game::NetworkSyncSystem sync;
    RType::Registry registry;
    AssetManager assets;
    MockAudioPlayer audio;
    std::vector<Game::NetworkClient::ServerMessage> messages {
        RType::Network::PlaySoundMsgPacket {.assetName = "sfx_explosion"}};

    sync.processServerMessages(
        messages, registry, assets, RType::Network::NO_ENTITY, nullptr);

    CHECK(audio.playCalls == 0);
}
