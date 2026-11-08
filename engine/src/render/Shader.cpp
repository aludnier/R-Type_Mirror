/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Shader.cpp
*/

#include "Shader.h"

#include <array>
#include <stdexcept>

#include <glad/glad.h>

namespace {
unsigned int compile(GLenum type, const std::string &source)
{
    const unsigned int shader = glCreateShader(type);
    const char *text = source.c_str();
    int success = 0;

    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == 0) {
        std::array<char, 1024> log {};

        glGetShaderInfoLog(shader, log.size(), nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error(
            std::string("cannot compile a shader: ") + log.data());
    }
    return shader;
}
} // namespace

namespace RType {
Shader::Shader(
    const std::string &vertexSource, const std::string &fragmentSource)
{
    const unsigned int vertex = compile(GL_VERTEX_SHADER, vertexSource);
    unsigned int fragment = 0;
    int success = 0;

    try {
        fragment = compile(GL_FRAGMENT_SHADER, fragmentSource);
    } catch (...) {
        glDeleteShader(vertex);
        throw;
    }
    _program = glCreateProgram();
    glAttachShader(_program, vertex);
    glAttachShader(_program, fragment);
    glLinkProgram(_program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glGetProgramiv(_program, GL_LINK_STATUS, &success);
    if (success == 0) {
        std::array<char, 1024> log {};

        glGetProgramInfoLog(_program, log.size(), nullptr, log.data());
        glDeleteProgram(_program);
        throw std::runtime_error(
            std::string("cannot link a shader: ") + log.data());
    }
}

Shader::~Shader()
{
    glDeleteProgram(_program);
}

void Shader::use() const
{
    glUseProgram(_program);
}

int Shader::uniform(const std::string &name) const
{
    return glGetUniformLocation(_program, name.c_str());
}
} // namespace RType
