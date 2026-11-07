/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** SceneRenderer.h
*/

#ifndef SCENERENDERER_H_
#define SCENERENDERER_H_

#include <cstdint>
#include <unordered_map>
#include <unordered_set>

#include "Renderer2D.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"
#include "library/RessourceManager.h"

namespace RType {
/** @brief Draws every entity with a Transform2D and a Sprite. */
class SceneRenderer {
public:
    explicit SceneRenderer(const ResourceManager &assets);

    /** @brief Draws the scene, sorted by zIndex. Skips missing assets. */
    void render(
        Registry &registry, const Window &target, const Camera2D &camera);

private:
    /** @return The texture of an asset, or nullptr if it cannot be loaded. */
    const Texture *texture(std::uint32_t assetId);

    const ResourceManager &_assets;
    Renderer2D _renderer;
    std::unordered_map<std::uint32_t, Texture> _textures;
    std::unordered_set<std::uint32_t> _missing;
};
} // namespace RType

#endif /* !SCENERENDERER_H_ */
