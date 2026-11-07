/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** RenderTarget.cpp
*/

#include "RenderTarget.h"

#include <glad/glad.h>

namespace RType {
void RenderTarget::clear(const Color &color)
{
    activate();
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}
} // namespace RType
