/*
** EPITECH PROJECT, 2026
** r-type
** File description:
** Renderer.h
*/

#ifndef RENDERER_H_
#define RENDERER_H_

namespace RType {
struct Color {
    float r = 0.f;
    float g = 0.f;
    float b = 0.f;
    float a = 1.f;
};

/** @brief Draws on the screen, so that games never call OpenGL directly. */
class Renderer {
public:
    /** @brief Fills the screen with a color. */
    static void clear(const Color &color);
};
} // namespace RType

#endif /* !RENDERER_H_ */
