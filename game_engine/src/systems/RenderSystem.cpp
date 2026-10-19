/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** RenderSystem implementation
*/

#include "systems/RenderSystem.h"
#include "components/Transform2D.h"
#include "components/Sprite.h"
#include "ecs/SparseArray.h"
#include "ecs/IndexedZipper.h"
#include "ecs/Zipper.h"
#include <algorithm>
#include <vector>

namespace RType {

std::optional<std::reference_wrapper<const sf::Texture> >
RenderSystem::loadTexture(const std::string &path)
{
    sf::Texture texture;
    if (!texture.loadFromFile(path)) {
        return std::nullopt;
    }
    auto [it, inserted] = _textureCache.emplace(path, std::move(texture));
    return std::cref(it->second);
}

std::optional<std::reference_wrapper<const sf::Texture> >
RenderSystem::getTexture(const std::string &path)
{
    auto it = _textureCache.find(path);
    if (it != _textureCache.end()) {
        return std::cref(it->second);
    }
    return loadTexture(path);
}

void RenderSystem::clearTextureCache()
{
    _textureCache.clear();
}

void RenderSystem::drawSprite(sf::RenderTarget &target,
                              const Transform2D &transform,
                              const Sprite &sprite, const sf::Texture &texture)
{
    sf::Sprite sfSprite(texture);
    if (sprite.rectWidth > 0 && sprite.rectHeight > 0) {
        sfSprite.setTextureRect(sf::IntRect(
            {sprite.rectLeft, sprite.rectTop},
            {sprite.rectWidth, sprite.rectHeight}
        ));
    }
    auto bounds = sfSprite.getLocalBounds();
    sfSprite.setOrigin(sf::Vector2f(
        bounds.size.x * sprite.originX,
        bounds.size.y * sprite.originY
    ));
    sfSprite.setPosition(
        sf::Vector2f(transform.position.x, transform.position.y));
    sfSprite.setScale(sf::Vector2f(transform.scale.x, transform.scale.y));
    sfSprite.setRotation(sf::radians(transform.rotation));
    sfSprite.setColor(sf::Color(sprite.r, sprite.g, sprite.b, sprite.a));

    target.draw(sfSprite);
}

namespace {
    struct RenderEntry {
        const Transform2D *transform;
        const Sprite *sprite;
    };
}

void RenderSystem::render(Registry &registry, const ResourceManager &resources,
                          sf::RenderTarget &target)
{
    auto &transforms = registry.getComponents<Transform2D>();
    auto &sprites = registry.getComponents<Sprite>();

    std::vector<RenderEntry> entries;

    for (auto [transform, sprite] : Zipper(transforms, sprites)) {
        entries.push_back({&transform, &sprite});
    }

    std::ranges::stable_sort(entries,
                             [](const RenderEntry &a, const RenderEntry &b) {
                                 return a.sprite->zIndex < b.sprite->zIndex;
                             });

    for (const auto &[transform, sprite] : entries) {

        std::optional<std::reference_wrapper<const sf::Texture> > textureOpt;
        auto assetOpt = resources.getAssetById(sprite->assetId);
        if (assetOpt.has_value() && !assetOpt->get().path.empty()) {
            textureOpt = getTexture(assetOpt->get().path);
        }

        if (textureOpt.has_value()) {
            drawSprite(target, *transform, *sprite, textureOpt->get());
        } else {
            sf::RectangleShape fallback(sf::Vector2f(
                32.0f * transform->scale.x,
                32.0f * transform->scale.y));
            fallback.setOrigin(sf::Vector2f(16.0f * transform->scale.x,
                                            16.0f * transform->scale.y));
            fallback.setPosition(
                sf::Vector2f(transform->position.x, transform->position.y));
            fallback.setRotation(sf::radians(transform->rotation));
            fallback.setFillColor(
                sf::Color(sprite->r, sprite->g, sprite->b, sprite->a));

            target.draw(fallback);
        }
    }
}

void addRenderSystem(Registry &registry, const ResourceManager &resources,
                     sf::RenderTarget &target, RenderSystem &renderSystem)
{
    registry.addSystem([&registry, &resources, &target, &renderSystem]() {
        renderSystem.render(registry, resources, target);
    });
}

} // namespace RType
