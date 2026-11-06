/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Shader.h
*/

#ifndef SHADER_H_
#define SHADER_H_

#include <string>

namespace RType {
/** @brief A vertex and fragment shader linked into one GPU program. */
class Shader {
public:
    /** @throws std::runtime_error if a shader does not compile or link. */
    Shader(const std::string &vertexSource, const std::string &fragmentSource);
    ~Shader();

    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;

    /** @brief Makes the next draw calls use this program. */
    void use() const;

    /** @return The location of a uniform, or -1 if it does not exist. */
    [[nodiscard]] int uniform(const std::string &name) const;

private:
    unsigned int _program = 0;
};
} // namespace RType

#endif /* !SHADER_H_ */
