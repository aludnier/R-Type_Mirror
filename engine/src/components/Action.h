/*
** EPITECH PROJECT, 2026
** r-type
** File description:
** Action
*/

#ifndef ACTION_H_
#define ACTION_H_

#include <cstdint>
#include <cstddef>

namespace RType {
using ActionId = std::uint8_t;
using ActionMask = std::uint32_t;
inline constexpr std::size_t MAX_ACTIONS = 32;
} // namespace RType

#endif /* !ACTION_H_ */
