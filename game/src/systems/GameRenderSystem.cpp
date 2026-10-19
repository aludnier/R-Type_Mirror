/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** GameRenderSystem implementation
*/

#include "systems/GameRenderSystem.h"
#include "ecs/Entity.h"
#include "ecs/SparseArray.h"
#include "ecs/Zipper.h"
#include "components/Transform2D.h"
#include "components/Sprite.h"
#include "components/Player.h"
#include "components/Enemy.h"
#include "components/Projectile.h"
#include "components/ForcePod.h"
#include "components/ParallaxBackground.h"
#include "systems/ParallaxSystem.h"
#include "components/Hitbox.h"
#include <cmath>

namespace Game {

    void GameRenderSystem::renderFrame(RType::Registry &registry, ResourceManager &resources, sf::RenderWindow &window)
    {
        window.clear(sf::Color(10, 10, 25));

        renderBackground(registry, window);
        _engineRenderSystem.render(registry, resources, window);
        renderForcePods(registry, window);
        renderPlayers(registry, window);
        renderEnemies(registry, window);
        renderProjectiles(registry, window);
        renderHud(registry, window);
    }

    void GameRenderSystem::renderBackground(RType::Registry &registry, sf::RenderWindow &window)
    {
        auto &transforms = registry.getComponents<RType::Transform2D>();
        auto &parallax = registry.getComponents<RType::Environment::ParallaxBackground>();

        for (auto [transform, bg] : RType::Zipper(transforms, parallax)) {
            (void)bg;
            sf::CircleShape star(2.0f);
            star.setFillColor(sf::Color(180, 180, 230, 150));
            for (int i = 0; i < 20; ++i) {
                auto x = std::fmod(transform.position.x + static_cast<float>(i * 100), 1920.0f);
                if (x < 0.0f) x += 1920.0f;
                auto y = static_cast<float>((i * 67) % 1080);
                star.setPosition(sf::Vector2f(x, y));
                window.draw(star);
            }
        }
    }

    void GameRenderSystem::renderForcePods(RType::Registry &registry, sf::RenderWindow &window)
    {
        auto &transforms = registry.getComponents<RType::Transform2D>();
        auto &forcePods = registry.getComponents<ForcePod>();

        for (auto [transform, pod] : RType::Zipper(transforms, forcePods)) {
            (void)pod;
            sf::CircleShape podShape(14.0f);
            podShape.setOrigin(sf::Vector2f(14.0f, 14.0f));
            podShape.setPosition(sf::Vector2f(transform.position.x, transform.position.y));
            podShape.setFillColor(sf::Color(255, 140, 0));
            podShape.setOutlineThickness(2.0f);
            podShape.setOutlineColor(sf::Color(255, 220, 100));
            window.draw(podShape);
        }
    }

    void GameRenderSystem::renderPlayers(RType::Registry &registry, sf::RenderWindow &window)
    {
        auto &transforms = registry.getComponents<RType::Transform2D>();
        auto &players = registry.getComponents<Player>();

        for (auto [transform, player] : RType::Zipper(transforms, players)) {
            sf::ConvexShape shipShape;
            shipShape.setPointCount(4);
            shipShape.setPoint(0, sf::Vector2f(25.0f, 0.0f));
            shipShape.setPoint(1, sf::Vector2f(-20.0f, -15.0f));
            shipShape.setPoint(2, sf::Vector2f(-12.0f, 0.0f));
            shipShape.setPoint(3, sf::Vector2f(-20.0f, 15.0f));
            shipShape.setFillColor(sf::Color(50, 150, 255));
            shipShape.setOutlineThickness(1.5f);
            shipShape.setOutlineColor(sf::Color(200, 230, 255));
            shipShape.setPosition(sf::Vector2f(transform.position.x, transform.position.y));
            window.draw(shipShape);

            if (player.chargeTime > 0.0f) {
                float chargeRatio = std::min(player.chargeTime / 1.5f, 1.0f);
                sf::CircleShape chargeGlow(6.0f + chargeRatio * 8.0f);
                chargeGlow.setOrigin(sf::Vector2f(chargeGlow.getRadius(), chargeGlow.getRadius()));
                chargeGlow.setPosition(sf::Vector2f(transform.position.x + 25.0f, transform.position.y));
                chargeGlow.setFillColor(sf::Color(100, 200, 255, static_cast<std::uint8_t>(100 + chargeRatio * 155)));
                window.draw(chargeGlow);
            }
        }
    }

    void GameRenderSystem::renderEnemies(RType::Registry &registry, sf::RenderWindow &window)
    {
        auto &transforms = registry.getComponents<RType::Transform2D>();
        auto &enemies = registry.getComponents<Enemy>();

        for (auto [transform, enemy] : RType::Zipper(transforms, enemies)) {
            (void)enemy;
            sf::ConvexShape enemyShape;
            enemyShape.setPointCount(5);
            enemyShape.setPoint(0, sf::Vector2f(-20.0f, 0.0f));
            enemyShape.setPoint(1, sf::Vector2f(15.0f, -15.0f));
            enemyShape.setPoint(2, sf::Vector2f(5.0f, -5.0f));
            enemyShape.setPoint(3, sf::Vector2f(5.0f, 5.0f));
            enemyShape.setPoint(4, sf::Vector2f(15.0f, 15.0f));
            enemyShape.setFillColor(sf::Color(220, 40, 60));
            enemyShape.setOutlineThickness(1.5f);
            enemyShape.setOutlineColor(sf::Color(255, 120, 140));
            enemyShape.setPosition(sf::Vector2f(transform.position.x, transform.position.y));
            window.draw(enemyShape);
        }
    }

    void GameRenderSystem::renderProjectiles(RType::Registry &registry, sf::RenderWindow &window)
    {
        auto &transforms = registry.getComponents<RType::Transform2D>();
        auto &projectiles = registry.getComponents<Projectile>();

        for (auto [transform, proj] : RType::Zipper(transforms, projectiles)) {
            if (!proj.isPlayerBullet) {
                sf::CircleShape bullet(4.0f);
                bullet.setOrigin(sf::Vector2f(4.0f, 4.0f));
                bullet.setPosition(sf::Vector2f(transform.position.x, transform.position.y));
                bullet.setFillColor(sf::Color(255, 80, 80));
                window.draw(bullet);
            } else if (proj.isChargedBeam) {
                sf::RectangleShape beam(sf::Vector2f(35.0f, 12.0f));
                beam.setOrigin(sf::Vector2f(17.5f, 6.0f));
                beam.setPosition(sf::Vector2f(transform.position.x, transform.position.y));
                beam.setFillColor(sf::Color(100, 220, 255));
                beam.setOutlineThickness(2.0f);
                beam.setOutlineColor(sf::Color(255, 255, 255));
                window.draw(beam);
            } else {
                sf::RectangleShape laser(sf::Vector2f(16.0f, 4.0f));
                laser.setOrigin(sf::Vector2f(8.0f, 2.0f));
                laser.setPosition(sf::Vector2f(transform.position.x, transform.position.y));
                laser.setFillColor(sf::Color(255, 230, 80));
                window.draw(laser);
            }
        }
    }

    void GameRenderSystem::renderHud(RType::Registry &registry, sf::RenderWindow &window)
    {
        auto &players = registry.getComponents<Player>();

        for (auto [player] : RType::Zipper(players)) {
            // Charge Meter Background
            sf::RectangleShape chargeBg(sf::Vector2f(200.0f, 14.0f));
            chargeBg.setPosition(sf::Vector2f(20.0f, 1040.0f));
            chargeBg.setFillColor(sf::Color(40, 40, 60));
            chargeBg.setOutlineThickness(1.5f);
            chargeBg.setOutlineColor(sf::Color(150, 150, 200));
            window.draw(chargeBg);

            // Charge Meter Fill
            if (player.chargeTime > 0.0f) {
                float chargeRatio = std::min(player.chargeTime / 1.5f, 1.0f);
                sf::RectangleShape chargeFill(sf::Vector2f(196.0f * chargeRatio, 10.0f));
                chargeFill.setPosition(sf::Vector2f(22.0f, 1042.0f));
                chargeFill.setFillColor(chargeRatio >= 1.0f ? sf::Color(100, 240, 255) : sf::Color(60, 160, 240));
                window.draw(chargeFill);
            }

            // Lives representation
            for (int l = 0; l < player.lives; ++l) {
                sf::ConvexShape miniShip;
                miniShip.setPointCount(3);
                miniShip.setPoint(0, sf::Vector2f(12.0f, 0.0f));
                miniShip.setPoint(1, sf::Vector2f(-6.0f, -6.0f));
                miniShip.setPoint(2, sf::Vector2f(-6.0f, 6.0f));
                miniShip.setFillColor(sf::Color(50, 150, 255));
                miniShip.setPosition(sf::Vector2f(30.0f + static_cast<float>(l * 24), 30.0f));
                window.draw(miniShip);
            }
            break; // HUD for main player
        }
    }

} // namespace Game
