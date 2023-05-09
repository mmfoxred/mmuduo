#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

/*
作用：
	从fd上读取数据  存入writableBytes()中；Poller工作在LT模式，所以要一次读完，不然会多次触发读事件。
	使用extrabuf就是防止一次读不完。
 */
ssize_t Buffer::readFd(int fd, int* saveErrno) {
    char extrabuf[65536] = {0};  // 栈上的内存空间  64K

    struct iovec vec[2];

    const size_t writableByte =
        writableBytes();  // 这是Buffer底层缓冲区剩余的可写空间大小
    vec[0].iov_base = begin() + m_writerIndex;
    vec[0].iov_len = writableByte;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writableByte < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0) {
        *saveErrno = errno;
    } else if (n <= writableByte)  // Buffer的可写缓冲区已经够存储读出来的数据了
    {
        m_writerIndex += n;
    } else  // extrabuf里面也写入了数据
    {
        m_writerIndex = m_buffer.size();
        append(extrabuf,
               n - writableByte);  // writerIndex_开始写 n - writable大小的数据
    }

    return n;
}

//调用write向fd写readableBytes中的数据
ssize_t Buffer::writeFd(int fd, int* saveErrno) {
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0) {
        *saveErrno = errno;
    }
    return n;
}