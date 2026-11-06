/*
** EPITECH PROJECT, 2026
** RType
** File description:
** SFML-backed game asset types
*/

#ifndef GAMEASSETTYPES_H_
#define GAMEASSETTYPES_H_

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

struct GameTextureAsset {
    static constexpr std::string_view JSON_TYPE = "Texture";
    sf::Texture resource;
    static GameTextureAsset load(const std::filesystem::path &path)
    {
        sf::Texture resource;
        if (!resource.loadFromFile(path.string()))
            throw std::runtime_error(
                "GameTextureAsset: failed to load '" + path.string() + "'");
        return GameTextureAsset {std::move(resource)};
    }
};

struct GameSoundAsset {
    static constexpr std::string_view JSON_TYPE = "Sound";
    sf::SoundBuffer resource;
    static GameSoundAsset load(const std::filesystem::path &path)
    {
        sf::SoundBuffer resource;
        if (!resource.loadFromFile(path.string()))
            throw std::runtime_error(
                "GameSoundAsset: failed to load '" + path.string() + "'");
        return GameSoundAsset {std::move(resource)};
    }
};

struct GameMusicAsset {
    static constexpr std::string_view JSON_TYPE = "Music";
    std::shared_ptr<sf::Music> resource;
    static GameMusicAsset load(const std::filesystem::path &path)
    {
        auto resource = std::make_shared<sf::Music>();
        if (!resource->openFromFile(path.string()))
            throw std::runtime_error(
                "GameMusicAsset: failed to load '" + path.string() + "'");
        return GameMusicAsset {std::move(resource)};
    }
};

struct GameFontAsset {
    static constexpr std::string_view JSON_TYPE = "Font";
    sf::Font resource;
    static GameFontAsset load(const std::filesystem::path &path)
    {
        sf::Font resource;
        if (!resource.openFromFile(path.string()))
            throw std::runtime_error(
                "GameFontAsset: failed to load '" + path.string() + "'");
        return GameFontAsset {std::move(resource)};
    }
};

#endif
