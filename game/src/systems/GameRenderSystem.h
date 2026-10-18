/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** GameRenderSystem header
*/

#ifndef GAME_RENDERSYSTEM_H
#define GAME_RENDERSYSTEM_H

#include "ecs/Registry.h"
#include "library/RessourceManager.h"
#include "systems/RenderSystem.h"
#include <SFML/Graphics.hpp>

namespace Game {

    /**
     * @brief System responsible for game visuals, HUD elements, and delegating to engine_client's RenderSystem.
     */
    class GameRenderSystem {
    public:
        /**
         * @brief Renders the current frame.
         */
        static void renderFrame(RType::Registry &registry, ResourceManager &resources, sf::RenderWindow &window);

    private:
        static void renderBackground(RType::Registry &registry, sf::RenderWindow &window);
        static void renderForcePods(RType::Registry &registry, sf::RenderWindow &window);
        static void renderPlayers(RType::Registry &registry, sf::RenderWindow &window);
        static void renderEnemies(RType::Registry &registry, sf::RenderWindow &window);
        static void renderProjectiles(RType::Registry &registry, sf::RenderWindow &window);
        static void renderHud(RType::Registry &registry, sf::RenderWindow &window);

        static inline RType::RenderSystem _engineRenderSystem;
    };

} // namespace Game

#endif // GAME_RENDERSYSTEM_H
