#include "CurrentThread.h"
#include <unistd.h>

namespace CurrentThread {
thread_local int t_cachedTid = 0;
void cacheTid() {
    if (t_cachedTid == 0) {
        //通过系统调用才能获得tid，所以要降低系统调用的次数，使用cacheTid
        t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
    }
}
}  // namespace CurrentThread