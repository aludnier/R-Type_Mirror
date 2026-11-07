#ifndef LIFETIME_TIMER_H
#define LIFETIME_TIMER_H

#include <string>

namespace RType::Lifetime {

struct Timer {
    float duration = 0.0f;
    float elapsed = 0.0f;
    std::string eventName;
    bool fired = false;
};

} // namespace RType::Lifetime

#endif
