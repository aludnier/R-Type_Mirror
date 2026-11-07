#ifndef LIFETIME_TIMER_EVENT_H
#define LIFETIME_TIMER_EVENT_H

#include <string>
#include <vector>

namespace RType::Lifetime {

struct TimerEvent {
    std::string name;
};

struct TimerEvents {
    std::vector<TimerEvent> events;
};


} // namespace RType::Lifetime

#endif
