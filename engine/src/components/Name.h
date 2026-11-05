/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Name component header
*/

#ifndef NAME_H
#define NAME_H

#include <string>
#include <utility>

namespace RType {

/**
 * @brief Human readable entity name, used by the editor and for debugging.
 */
struct Name {
    std::string value;

    Name() = default;

    explicit Name(std::string value) :
        value(std::move(value))
    {
    }

    bool operator==(const Name &o) const = default;
};

} // namespace RType

#endif // NAME_H
