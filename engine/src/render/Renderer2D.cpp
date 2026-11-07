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

uniform vec2 viewCenter;
uniform vec2 viewHalfSize;
uniform vec2 position;
uniform vec2 size;
uniform vec2 origin;
uniform float rotation;
uniform vec4 textureRect;

out vec2 textureCoords;

void main()
{
    vec2 local = (corner - origin) * size;
    float c = cos(rotation);
    float s = sin(rotation);
    vec2 world = position + vec2(local.x * c - local.y * s,
        local.x * s + local.y * c);
    vec2 screen = (world - viewCenter) / viewHalfSize;

    gl_Position = vec4(screen.x, -screen.y, 0.0, 1.0);
    textureCoords = textureRect.xy + corner * textureRect.zw;
}
)";

constexpr const char *FRAGMENT_SHADER = R"(#version 330 core
in vec2 textureCoords;

uniform sampler2D image;
uniform vec4 tint;

out vec4 color;

void main()
{
    color = texture(image, textureCoords) * tint;
}
)";

// Corners of the rectangle, as two triangles. y goes down, like in an image.
constexpr std::array<float, 12> CORNERS = {
    0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f};
} // namespace

namespace RType {
Renderer2D::Renderer2D() :
    _shader(VERTEX_SHADER, FRAGMENT_SHADER),
    _viewCenter(_shader.uniform("viewCenter")),
    _viewHalfSize(_shader.uniform("viewHalfSize")),
    _position(_shader.uniform("position")),
    _size(_shader.uniform("size")),
    _origin(_shader.uniform("origin")),
    _rotation(_shader.uniform("rotation")),
    _textureRect(_shader.uniform("textureRect")),
    _tint(_shader.uniform("tint"))
{
    glGenVertexArrays(1, &_vertexArray);
    glGenBuffers(1, &_vertexBuffer);
    glBindVertexArray(_vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    glBufferData(
        GL_ARRAY_BUFFER, sizeof(CORNERS), CORNERS.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
}

Renderer2D::~Renderer2D()
{
    glDeleteBuffers(1, &_vertexBuffer);
    glDeleteVertexArrays(1, &_vertexArray);
}

void Renderer2D::begin(const Window &target, const Camera2D &camera)
{
    const float halfWidth = static_cast<float>(target.width()) / 2.f;
    const float halfHeight = static_cast<float>(target.height()) / 2.f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    _shader.use();
    glBindVertexArray(_vertexArray);
    glUniform2f(_viewCenter, camera.center.x, camera.center.y);
    glUniform2f(
        _viewHalfSize, halfWidth / camera.zoom, halfHeight / camera.zoom);
}

void Renderer2D::drawSprite(const Texture &texture,
    const Transform2D &transform, const Sprite &sprite) const
{
    const auto textureWidth = static_cast<float>(texture.width());
    const auto textureHeight = static_cast<float>(texture.height());
    const float width = sprite.rectWidth != 0
        ? static_cast<float>(sprite.rectWidth)
        : textureWidth;
    const float height = sprite.rectHeight != 0
        ? static_cast<float>(sprite.rectHeight)
        : textureHeight;

    glUniform2f(_position, transform.position.x, transform.position.y);
    glUniform2f(_size, width * transform.scale.x, height * transform.scale.y);
    glUniform2f(_origin, sprite.originX, sprite.originY);
    glUniform1f(_rotation, transform.rotation);
    glUniform4f(_textureRect,
        static_cast<float>(sprite.rectLeft) / textureWidth,
        static_cast<float>(sprite.rectTop) / textureHeight,
        width / textureWidth,
        height / textureHeight);
    glUniform4f(_tint,
        static_cast<float>(sprite.r) / 255.f,
        static_cast<float>(sprite.g) / 255.f,
        static_cast<float>(sprite.b) / 255.f,
        static_cast<float>(sprite.a) / 255.f);
    texture.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer2D::end()
{
    glBindVertexArray(0);
    glUseProgram(0);
}
} // namespace RType
