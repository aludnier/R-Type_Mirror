/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Framebuffer.cpp
*/

#include "Framebuffer.h"

#include <stdexcept>

#include <glad/glad.h>

namespace RType {
Framebuffer::Framebuffer(int width, int height)
{
    glGenFramebuffers(1, &_id);
    glGenTextures(1, &_texture);
    glBindTexture(GL_TEXTURE_2D, _texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    resize(width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, _id);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texture, 0);
    const bool complete =
        glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (!complete) {
        glDeleteTextures(1, &_texture);
        glDeleteFramebuffers(1, &_id);
        throw std::runtime_error("cannot create the framebuffer");
    }
}

Framebuffer::~Framebuffer()
{
    glDeleteTextures(1, &_texture);
    glDeleteFramebuffers(1, &_id);
}

void Framebuffer::activate()
{
    glBindFramebuffer(GL_FRAMEBUFFER, _id);
    glViewport(0, 0, _width, _height);
}

int Framebuffer::width() const
{
    return _width;
}

int Framebuffer::height() const
{
    return _height;
}

void Framebuffer::resize(int width, int height)
{
    if (width <= 0 || height <= 0 || (width == _width && height == _height))
        return;

    _width = width;
    _height = height;
    glBindTexture(GL_TEXTURE_2D, _texture);
    glTexImage2D(GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        _width,
        _height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr);
}

unsigned int Framebuffer::textureId() const
{
    return _texture;
}
} // namespace RType
