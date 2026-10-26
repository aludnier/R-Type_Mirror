/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** RenderSystem header
*/

#ifndef ENGINE_RENDERSYSTEM_H
#define ENGINE_RENDERSYSTEM_H

#include "ecs/Registry.h"
#include "library/RessourceManager.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>
#include <optional>
#include <functional>

#include "components/Sprite.h"
#include "components/Transform2D.h"

namespace RType {

    /**
     * @brief System responsible for 2D graphics rendering using SFML.
     * Manages texture caching and sprite rendering according to Transform2D and Sprite components.
     */
    class RenderSystem {
    public:
        RenderSystem() = default;
        ~RenderSystem() = default;

        /**
         * @brief Loads and caches a texture from a file path.
         * @param path Filepath to the texture asset.
         * @return Optional reference to the cached texture, or std::nullopt if loading failed.
         */
        std::optional<std::reference_wrapper<const sf::Texture>> loadTexture(const std::string &path);

        /**
         * @brief Gets a cached texture or loads it if not already cached.
         * @param path Filepath to the texture asset.
         * @return Optional reference to the texture, or std::nullopt if unavailable.
         */
        std::optional<std::reference_wrapper<const sf::Texture>> getTexture(const std::string &path);

        /**
         * @brief Clears the internal texture cache.
         */
        void clearTextureCache();

        static void drawSprite(sf::RenderTarget &target, const Transform2D &transform,
                               const Sprite &sprite, const sf::Texture &texture);

        /**
         * @brief Renders all entities with Transform2D and Sprite components to the provided render target.
         * @param registry The ECS registry containing entity components.
         * @param resources The resource manager holding asset metadata.
         * @param target The target SFML RenderTarget (e.g. sf::RenderWindow, sf::RenderTexture).
         */
        void render(Registry &registry, const ResourceManager &resources, sf::RenderTarget &target);

    private:
        std::unordered_map<std::string, sf::Texture> _textureCache;
    };

    /**
     * @brief Registers the render system into the ECS registry.
     */
    void addRenderSystem(Registry &registry, const ResourceManager &resources, sf::RenderTarget &target, RenderSystem &renderSystem);

} // namespace RType

#endif // ENGINE_RENDERSYSTEM_H
