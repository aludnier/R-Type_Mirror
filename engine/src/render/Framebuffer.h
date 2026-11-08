/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Framebuffer.h
*/

#ifndef FRAMEBUFFER_H_
#define FRAMEBUFFER_H_

#include "platform/RenderTarget.h"

namespace RType {
/** @brief An image on the GPU that the renderer can draw to. */
class Framebuffer : public RenderTarget {
public:
    /** @throws std::runtime_error if the framebuffer cannot be made. */
    Framebuffer(int width, int height);
    ~Framebuffer() override;

    void activate() override;
    [[nodiscard]] int width() const override;
    [[nodiscard]] int height() const override;

    /** @brief Changes the size of the image. Ignores empty sizes. */
    void resize(int width, int height);
    /** @return The OpenGL texture holding the image. */
    [[nodiscard]] unsigned int textureId() const;

private:
    unsigned int _id = 0;
    unsigned int _texture = 0;
    int _width = 0;
    int _height = 0;
};
} // namespace RType

#endif /* !FRAMEBUFFER_H_ */
