/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ProductVersion.hpp
*/
#ifndef PRODUCTVERSION_HPP
#define PRODUCTVERSION_HPP

namespace RType {
class ProductVersion {
public:
    constexpr ProductVersion(int major, int minor, int patch)
        : _major(major), _minor(minor), _patch(patch) {}

    [[nodiscard]] int getMajor() const { return _major; }
    [[nodiscard]] int getMinor() const { return _minor; }
    [[nodiscard]] int getPatch() const { return _patch; }
private:
    int _major;
    int _minor;
    int _patch;
};
} // RType

#endif
