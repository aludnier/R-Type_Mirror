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

/** @brief Something the renderer can draw to. */
class RenderTarget {
public:
    RenderTarget() = default;
    virtual ~RenderTarget() = default;

    RenderTarget(const RenderTarget &) = delete;
    RenderTarget &operator=(const RenderTarget &) = delete;

    /** @brief Makes the next draw calls go to this target. */
    virtual void activate() = 0;
    [[nodiscard]] virtual int width() const = 0;
    [[nodiscard]] virtual int height() const = 0;

    /** @brief Fills the target with a color. */
    void clear(const Color &color);
};
} // namespace RType

#endif /* !RENDERTARGET_H_ */
