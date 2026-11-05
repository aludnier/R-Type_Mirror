/*
** EPITECH PROJECT, 2026
** R-type
** File description:
** main.cpp
*/

#include <exception>
#include <iostream>

#include "library/Keys.h"
#include "platform/Application.h"

namespace {
constexpr RType::ActionId Quit = 0;

class Editor : public RType::Application {
public:
    Editor() :
        Application({.title = "Editor"})
    {
        input().bind(Quit, RType::Key::Escape);
    }

protected:
    void update(float /*dt*/) override
    {
        if (input().isActionDown(Quit))
            window().close();
    }

    void render() override
    {
        window().clear({0.12f, 0.12f, 0.14f, 1.f});
    }
};
} // namespace

int main()
{
    try {
        Editor editor;

        editor.run();
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
