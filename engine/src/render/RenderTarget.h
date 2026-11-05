/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** RenderTarget.h
*/

#ifndef RENDERTARGET_H_
#define RENDERTARGET_H_

namespace RType {
struct Color {
    float r = 0.f;
    float g = 0.f;
    float b = 0.f;
    float a = 1.f;
};

/** @brief Something that can be drawn on: a window, later a framebuffer. */
class RenderTarget {
public:
    virtual ~RenderTarget() = default;

    /** @brief Fills this target with a color. */
    void clear(const Color &color);

protected:
    /** @brief Makes OpenGL draw on this target. */
    virtual void activate() = 0;
};
} // namespace RType

#endif /* !RENDERTARGET_H_ */
