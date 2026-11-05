/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Renderer2D.h
*/

#ifndef RENDERER2D_H_
#define RENDERER2D_H_

#include "Shader.h"
#include "Texture.h"

namespace RType {
/** @brief A rectangle in screen space: -1 to 1 on both axes, y going up. */
struct ScreenRect {
    float left = -1.f;
    float top = 1.f;
    float width = 2.f;
    float height = 2.f;
};

/** @brief Draws textured rectangles. Needs an active OpenGL context. */
class Renderer2D {
public:
    Renderer2D();
    ~Renderer2D();

    Renderer2D(const Renderer2D &) = delete;
    Renderer2D &operator=(const Renderer2D &) = delete;

    void drawTexture(const Texture &texture, const ScreenRect &rect);

private:
    Shader _shader;
    int _rectUniform = -1;
    unsigned int _vertexArray = 0;
    unsigned int _vertexBuffer = 0;
};
} // namespace RType

#endif /* !RENDERER2D_H_ */
