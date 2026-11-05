/*
** EPITECH PROJECT, 2026
** r-type
** File description:
** Renderer.cpp
*/

#include "Renderer.h"

#include <glad/glad.h>

namespace RType {
void Renderer::clear(const Color &color)
{
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}
} // namespace RType
