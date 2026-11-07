/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Renderer2D.h
*/

#ifndef RENDERER2D_H_
#define RENDERER2D_H_

#include "Camera2D.h"
#include "Shader.h"
#include "Texture.h"
#include "components/Sprite.h"
#include "components/Transform2D.h"
#include "platform/RenderTarget.h"

namespace RType {
/** @brief Draws sprites. Needs an active OpenGL context. */
class Renderer2D {
public:
    Renderer2D();
    ~Renderer2D();

    Renderer2D(const Renderer2D &) = delete;
    Renderer2D &operator=(const Renderer2D &) = delete;

    /** @brief Starts drawing to a target, as seen by a camera. */
    void begin(RenderTarget &target, const Camera2D &camera);
    void drawSprite(const Texture &texture, const Transform2D &transform,
        const Sprite &sprite) const;
    static void end();

private:
    Shader _shader;
    int _viewCenter = -1;
    int _viewHalfSize = -1;
    int _position = -1;
    int _size = -1;
    int _origin = -1;
    int _rotation = -1;
    int _textureRect = -1;
    int _tint = -1;
    unsigned int _vertexArray = 0;
    unsigned int _vertexBuffer = 0;
};
} // namespace RType

#endif /* !RENDERER2D_H_ */
