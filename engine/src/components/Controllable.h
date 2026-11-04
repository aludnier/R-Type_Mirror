#ifndef CONTROLLABLE_H
#define CONTROLLABLE_H

#include <cstdint>

namespace RType {
/** @brief Input actions currently active for an entity. */
struct Controllable {
    std::uint8_t actionMask = 0;

    constexpr Controllable() = default;

    constexpr explicit Controllable(std::uint8_t actionMask) :
        actionMask(actionMask)
    {
    }

    constexpr bool operator==(const Controllable &o) const = default;
};
} // namespace RType

#endif
