/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Texture.h
*/

#ifndef TEXTURE_H_
#define TEXTURE_H_

#include <filesystem>

namespace RType {
/** @brief An image stored on the GPU. */
class Texture {
public:
    /** @throws std::runtime_error if the image cannot be loaded. */
    explicit Texture(const std::filesystem::path &file);
    ~Texture();

    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;

    /** @brief Makes the next draw calls sample this texture. */
    void bind() const;

    [[nodiscard]] int width() const;
    [[nodiscard]] int height() const;

private:
    unsigned int _id = 0;
    int _width = 0;
    int _height = 0;
};
} // namespace RType

#endif /* !TEXTURE_H_ */
