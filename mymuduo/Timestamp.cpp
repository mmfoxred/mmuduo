#include "Timestamp.h"

#include <cstddef>
#include <ctime>
Timestamp::Timestamp() : m_secondSinceEpoch(0) {}

Timestamp::Timestamp(long secondsSinceEpoch)
    : m_secondSinceEpoch(secondsSinceEpoch) {}

Timestamp Timestamp::now() { return Timestamp(time(NULL)); }

// 这个是给连续调用使用的，不用关心是否是当前的时间
const std::string Timestamp::toString() const {
    char strbuf[80] = {0};
    tm* t = localtime(&m_secondSinceEpoch);  // 可以用Long代替time_t
    strftime(strbuf, 80, "%Y-%m-%d %H:%M:%S ", t);
    return strbuf;
}
