/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Configuration.hpp
*/

#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <nlohmann/json.hpp>

namespace RType {
    class Configuration {
    public:
        explicit Configuration(const std::string &configFilePath);
        ~Configuration() = default;
    private:
        std::string _name;

    };
} // RType

#endif
