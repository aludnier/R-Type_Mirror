#ifndef CONTROLLABLE_H
#define CONTROLLABLE_H

#include <cstdint>
#include "components/Action.h"

namespace RType {
/** @brief Input actions currently active for an entity. */
struct Controllable {
    ActionMask actionMask = 0;

    constexpr Controllable() = default;

    constexpr explicit Controllable(ActionMask actionMask) :
        actionMask(actionMask)
    {
    }

    constexpr bool operator==(const Controllable &o) const = default;
};
} // namespace RType

#endif
