#include "Timestamp.h"

#include <sys/time.h>
#include <cstddef>
#include <ctime>
//返回是其实还是一个long型时间戳，需要toString()的转换
Timestamp Timestamp::now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}

// 这个是给连续调用使用的，不用关心是否是当前的时间
const std::string Timestamp::toString() const {
    char buf[32] = {0};
    int64_t seconds = m_microSecondsSinceEpoch / kMicroSecondsPerSecond;
    int64_t microseconds = m_microSecondsSinceEpoch % kMicroSecondsPerSecond;
    snprintf(buf, sizeof(buf), "%ld.%ld", seconds, microseconds);
    return buf;
}
