#include "CircularBuffer.h"
#include <errno.h>
#include <stdio.h>
#include <sys/uio.h>
#include "SocketOps.h"
#include <unistd.h>

const char CircularBuffer::kCRLF[] = "\r\n";

const size_t CircularBuffer::kCheapPrepend;
const size_t CircularBuffer::kInitialSize;

ssize_t CircularBuffer::readFd(int fd, int* savedErrno) {
    // saved an ioctl()/FIONREAD call to tell how much to read
    char extrabuf[65536];
    struct iovec vec[2];
    size_t writable;
    if (m_writerIndex > m_readerIndex) {
        writable = m_buffer.size() - m_writerIndex;
    } else {
        writable = writableBytes();
    }

    vec[0].iov_base = begin() + m_writerIndex;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    const ssize_t n = readv(fd, vec, 2);
    if (n < 0) {
        *savedErrno = errno;
    } else if (implicit_cast<size_t>(n) <= writable) {
        m_writerIndex += static_cast<unsigned long>(n);
        m_hasData = true;
    } else {
        m_writerIndex = m_buffer.size();
        append(extrabuf, static_cast<unsigned long>(n) - writable);
    }
    // if (n == writable + sizeof extrabuf)
    // {
    //   goto line_30;
    // }
    // printf("buffer_size: %d, writable: %d, n: %d\n", m_buffer.size(), n,
    //        writable);
    return n;
}

//调用write向fd写readableBytes中的数据
ssize_t CircularBuffer::writeFd(int fd, int* saveErrno) {
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0) {
        *saveErrno = errno;
    }
    return n;
}