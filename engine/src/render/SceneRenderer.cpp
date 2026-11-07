/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** SceneRenderer.cpp
*/

#include "SceneRenderer.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <vector>

namespace RType {
SceneRenderer::SceneRenderer(const AssetManager &assets) :
    _assets(assets)
{
}

void SceneRenderer::render(
    Registry &registry, RenderTarget &target, const Camera2D &camera)
{
    if (!registry.isRegistered<Transform2D>() ||
        !registry.isRegistered<Sprite>())
        return;

    auto &transforms = registry.getComponents<Transform2D>();
    auto &sprites = registry.getComponents<Sprite>();
    std::vector<entity_id_type> visible;

    for (const Entity &entity : registry.entities()) {
        const auto id = static_cast<entity_id_type>(entity);

        if (id < transforms.size() && id < sprites.size() &&
            transforms[id].has_value() && sprites[id].has_value())
            visible.push_back(id);
    }
    std::ranges::stable_sort(
        visible, {}, [&](entity_id_type id) { return sprites[id]->zIndex; });

    _renderer.begin(target, camera);
    for (const entity_id_type id : visible) {
        const Texture *image = texture(sprites[id]->assetId);

        if (image != nullptr)
            _renderer.drawSprite(*image, *transforms[id], *sprites[id]);
    }
    Renderer2D::end();
}

const Texture *SceneRenderer::texture(std::uint32_t assetId)
{
    if (const auto it = _textures.find(assetId); it != _textures.end())
        return &it->second;
    if (_missing.contains(assetId))
        return nullptr;

    const auto asset = _assets.getAssetById(assetId);

    try {
        if (asset.has_value())
            return &_textures.try_emplace(assetId, asset->get().path)
                        .first->second;
        std::cerr << "unknown asset id " << assetId << "\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << "\n";
    }
    _missing.insert(assetId);
    return nullptr;
}
} // namespace RType
