/*
** EPITECH PROJECT, 2026
** r-type
** File description:
** Action
*/

#ifndef ACTION_H_
#define ACTION_H_

#include <cstdint>

namespace RType {
    using ActionId = std::uint8_t;
    using ActionMask = std::uint32_t;
}

enum class Action : std::uint8_t {
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Shoot,
    Count
};

#endif /* !ACTION_H_ */
