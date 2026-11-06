/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Renderer2D.cpp
*/

#include "Renderer2D.h"

#include <array>

#include <glad/glad.h>

namespace {
constexpr const char *VERTEX_SHADER = R"(#version 330 core
layout(location = 0) in vec2 corner;

uniform vec4 rect;

out vec2 textureCoords;

void main()
{
    vec2 position = rect.xy + vec2(corner.x * rect.z, -corner.y * rect.w);

    gl_Position = vec4(position, 0.0, 1.0);
    textureCoords = corner;
}
)";

constexpr const char *FRAGMENT_SHADER = R"(#version 330 core
in vec2 textureCoords;

uniform sampler2D image;

out vec4 color;

void main()
{
    color = texture(image, textureCoords);
}
)";

// Corners of the rectangle, as two triangles. y goes down, like in an image.
constexpr std::array<float, 12> CORNERS = {
    0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f};
} // namespace

namespace RType {
Renderer2D::Renderer2D() :
    _shader(VERTEX_SHADER, FRAGMENT_SHADER),
    _rectUniform(_shader.uniform("rect"))
{
    glGenVertexArrays(1, &_vertexArray);
    glGenBuffers(1, &_vertexBuffer);
    glBindVertexArray(_vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    glBufferData(
        GL_ARRAY_BUFFER, sizeof(CORNERS), CORNERS.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

Renderer2D::~Renderer2D()
{
    glDeleteBuffers(1, &_vertexBuffer);
    glDeleteVertexArrays(1, &_vertexArray);
}

void Renderer2D::drawTexture(const Texture &texture, const ScreenRect &rect)
{
    _shader.use();
    glUniform4f(_rectUniform, rect.left, rect.top, rect.width, rect.height);
    texture.bind();
    glBindVertexArray(_vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
} // namespace RType
