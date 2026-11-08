#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>

#include "GameAssetTypes.h"
#include "TempConfigFile.hpp"
#include "audio/AudioManager.h"

namespace {

void writeUint16(std::ofstream &output, std::uint16_t value)
{
    output.put(static_cast<char>(value & 0xFF));
    output.put(static_cast<char>((value >> 8) & 0xFF));
}

void writeUint32(std::ofstream &output, std::uint32_t value)
{
    writeUint16(output, static_cast<std::uint16_t>(value & 0xFFFF));
    writeUint16(output, static_cast<std::uint16_t>(value >> 16));
}

void writeSilentWave(const std::filesystem::path &path)
{
    constexpr std::uint32_t sampleRate = 8000;
    constexpr std::uint32_t dataSize = sampleRate * sizeof(std::int16_t);
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    REQUIRE(output.good());
    output.write("RIFF", 4);
    writeUint32(output, 36 + dataSize);
    output.write("WAVEfmt ", 8);
    writeUint32(output, 16);
    writeUint16(output, 1);
    writeUint16(output, 1);
    writeUint32(output, sampleRate);
    writeUint32(output, sampleRate * sizeof(std::int16_t));
    writeUint16(output, sizeof(std::int16_t));
    writeUint16(output, 16);
    output.write("data", 4);
    writeUint32(output, dataSize);
    for (std::uint32_t i = 0; i < sampleRate; ++i)
        writeUint16(output, 0);
    REQUIRE(output.good());
}

class AudioFixture {
public:
    AudioFixture() :
        _soundPath(makeSoundPath()), _config(makeConfig(_soundPath))
    {
        writeSilentWave(_soundPath);
    }

    ~AudioFixture()
    {
        std::error_code error;
        std::filesystem::remove(_soundPath, error);
    }

    [[nodiscard]] const std::filesystem::path &soundPath() const
    {
        return _soundPath;
    }

    [[nodiscard]] const std::filesystem::path &configPath() const
    {
        return _config.path();
    }

private:
    static std::filesystem::path makeSoundPath()
    {
        static std::size_t counter = 0;
        return std::filesystem::temp_directory_path() /
            ("rtype_audio_test_" + std::to_string(counter++) + ".wav");
    }

    static std::string makeConfig(const std::filesystem::path &soundPath)
    {
        const auto path = soundPath.string();
        return "{\"assets\":{"
               "\"sound\":{\"path\":\"" +
            path +
            "\",\"type\":\"Sound\"},"
            "\"music\":{\"path\":\"" +
            path + R"(","type":"Music"}}})";
    }

    std::filesystem::path _soundPath;
    TempConfigFile _config;
};

ClientAssetManager createAudioAssets(const AudioFixture &fixture)
{
    ClientAssetManager assets(fixture.configPath());
    assets.registerType<GameSoundAsset>();
    assets.registerType<GameMusicAsset>();
    return assets;
}

} // namespace

TEST_CASE("AudioManager handles voices and clamps volume channels", "[audio]")
{
    AudioFixture fixture;
    auto assets = createAudioAssets(fixture);
    RType::Game::AudioManager audio(assets, 1);

    const auto first = audio.playSound("sound");
    REQUIRE(first.index == 0);
    REQUIRE(first.generation != 0);
    audio.stopSound(first);

    const auto recycled = audio.playSound("sound");
    REQUIRE(recycled.index == first.index);
    REQUIRE(recycled.generation != first.generation);
    audio.stopSound(first);
    audio.stopSound(recycled);

    audio.setVolume(RType::AudioChannel::Master, 1.5f);
    audio.setVolume(RType::AudioChannel::Music, 0.35f);
    audio.setVolume(RType::AudioChannel::Sound, -0.5f);
    REQUIRE(audio.getVolume(RType::AudioChannel::Master) == 1.f);
    REQUIRE(audio.getVolume(RType::AudioChannel::Music) == 0.35f);
    REQUIRE(audio.getVolume(RType::AudioChannel::Sound) == 0.f);

    audio.playMusic("music", false, 1.5f);
    audio.pauseMusic();
    audio.resumeMusic();
    audio.stopMusic();
    audio.stopAll();
}

TEST_CASE("AudioManager reports missing and mistyped assets without throwing",
    "[audio]")
{
    AudioFixture fixture;
    auto assets = createAudioAssets(fixture);
    RType::Game::AudioManager audio(assets);

    REQUIRE(audio.playSound("unknown").index ==
        std::numeric_limits<std::uint32_t>::max());
    REQUIRE(audio.playSound("music").index ==
        std::numeric_limits<std::uint32_t>::max());
    audio.playMusic("sound");
    REQUIRE_FALSE(audio.isMusicPlaying());
}

TEST_CASE("AudioManager uses the cached sound buffer on later plays", "[audio]")
{
    AudioFixture fixture;
    auto assets = createAudioAssets(fixture);
    RType::Game::AudioManager audio(assets);

    const auto first = audio.playSound("sound");
    REQUIRE(first.index != std::numeric_limits<std::uint32_t>::max());
    std::filesystem::remove(fixture.soundPath());
    const auto second = audio.playSound("sound");
    REQUIRE(second.index != std::numeric_limits<std::uint32_t>::max());
    audio.stopAll();
}
