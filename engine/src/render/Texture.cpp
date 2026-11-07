/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Texture.cpp
*/

#include "Texture.h"

#include <stdexcept>

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace RType {
Texture::Texture(const std::filesystem::path &file)
{
    int channels = 0;
    unsigned char *pixels = stbi_load(
        file.string().c_str(), &_width, &_height, &channels, STBI_rgb_alpha);

    if (pixels == nullptr)
        throw std::runtime_error("cannot load the image " + file.string());

    glGenTextures(1, &_id);
    glBindTexture(GL_TEXTURE_2D, _id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        _width,
        _height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels);
    stbi_image_free(pixels);
}

Texture::~Texture()
{
    glDeleteTextures(1, &_id);
}

void Texture::bind() const
{
    glBindTexture(GL_TEXTURE_2D, _id);
}

int Texture::width() const
{
    return _width;
}

int Texture::height() const
{
    return _height;
}
} // namespace RType
