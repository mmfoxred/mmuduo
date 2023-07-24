#include "Timer.h"
#include <atomic>

std::atomic<int> Timer::s_numCreated_;

void Timer::restart(Timestamp now) {
    if (repeat_) {
        expiration_ = addTime(now, interval_);
    } else {
        expiration_ = Timestamp::invalid();
    }
}
