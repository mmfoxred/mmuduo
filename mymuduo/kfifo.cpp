#include "kfifo.h"
#include <sys/uio.h>
#include <unistd.h>
#include <cstring>

Kfifo::Kfifo() {
    buffer = nullptr;
    bufferSize = 0;
    write = 0;
    read = 0;
}

Kfifo::~Kfifo() {
    freeBuffer();
}

bool Kfifo::initBuffer(uint32_t size) {
    //需要保证为2的次幂 取余运算转换为与运算 提升效率，即write%bufferSize == write &(bufferSize-1)
    if (!is_power_of_two(size)) {
        if (size < 2)
            size = 2;

        //向上取2的次幂
        int i = 0;
        for (; size != 0; i++)
            size >>= 1;

        size = 1U << i;
    }

    std::lock_guard<std::mutex> lock(mutex);
    buffer = new uint8_t[size];
    if (buffer == nullptr)
        return false;

    memset(buffer, 0, size);
    bufferSize = size;
    write = 0;
    read = 0;
    return true;
}

void Kfifo::freeBuffer() {
    std::lock_guard<std::mutex> lock(mutex);
    bufferSize = 0;
    write = 0;
    read = 0;

    if (buffer != nullptr) {
        delete[] buffer;
        buffer = nullptr;
    }
}

void Kfifo::resetBuffer() {
    std::lock_guard<std::mutex> lock(mutex);
    write = 0;
    read = 0;
    memset(buffer, 0, bufferSize);
}

bool Kfifo::isEmpty() {
    std::lock_guard<std::mutex> lock(mutex);
    return write == read;
}

bool Kfifo::isFull() {
    std::lock_guard<std::mutex> lock(mutex);
    return bufferSize == (write - read);
}

uint32_t Kfifo::getReadableLen() {
    std::lock_guard<std::mutex> lock(mutex);
    return write - read;
}

uint32_t Kfifo::getRemainLen() {
    std::lock_guard<std::mutex> lock(mutex);
    // return bufferSize - (write - read);
    return bufferSize - (write - read);
}

uint32_t Kfifo::getBufferSize() {
    std::lock_guard<std::mutex> lock(mutex);
    return bufferSize;
}

uint32_t Kfifo::writeBuffer(char* inBuf, uint32_t inSize) {
    std::lock_guard<std::mutex> lock(mutex);

    if (buffer == nullptr || inBuf == nullptr || inSize == 0)
        return -1;

    //写入数据大小和缓冲区剩余空间大小 取最小值为最终写入大小
    inSize = Min(inSize, getRemainLen());

    //写数据如果写到末尾仍未写完的情况，那么回到头部继续写
    uint32_t len = Min(inSize, bufferSize - (write & (bufferSize - 1)));
    //区间为写指针位置到缓冲区末端
    memcpy(buffer + (write & (bufferSize - 1)), inBuf, len);
    //回到缓冲区头部继续写剩余数据
    memcpy(buffer, inBuf + len, inSize - len);

    //无符号溢出则为 0
    write += inSize;
    return inSize;
}

uint32_t Kfifo::readBuffer(char* outBuf, uint32_t outSize) {
    std::lock_guard<std::mutex> lock(mutex);

    if (buffer == nullptr || outBuf == nullptr || outSize == 0)
        return -1;

    //读出数据大小和缓冲区可读数据大小 取最小值为最终读出大小
    outSize = Min(outSize, getReadableLen());

    //读数据如果读到末尾仍未读完的情况, 那么回到头部继续读
    uint32_t len = Min(outSize, bufferSize - (read & (bufferSize - 1)));
    //区间为读指针位置到缓冲区末端
    memcpy(outBuf, buffer + (read & (bufferSize - 1)), len);
    //回到缓冲区头部继续读剩余数据
    memcpy(outBuf + len, buffer, outSize - len);

    //无符号溢出则为 0
    read += outSize;
    return outSize;
}

/*
作用：
    从fd上读取数据 存入writableBytes()中；Poller工作在LT模式，所以要一次读完，不然会多次触发读事件。
    使用extrabuf就是防止一次读不完。
 */
ssize_t Kfifo::readFd(int fd, int* saveErrno) {
    char extrabuf[65536] = {0};  // 栈上的内存空间  64K

    struct iovec vec[2];

    // 这是Buffer底层缓冲区剩余的可写空间大小
    const size_t writableByte = bufferSize - (write & (bufferSize - 1));
    vec[0].iov_base = buffer + (write & (bufferSize - 1));
    vec[0].iov_len = writableByte;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writableByte < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0) {
        *saveErrno = errno;
    } else if (n <= writableByte)  // Buffer的可写缓冲区已经够存储读出来的数据了
    {
        write += n;
    } else  // extrabuf里面也写入了数据
    {
        writeBuffer(extrabuf, n - writableByte);
    }

    return n;
}

//调用write向fd写readableBytes中的数据
ssize_t Kfifo::writeFd(int fd, int* saveErrno) {
    ssize_t n = ::write(fd, buffer + (read & (bufferSize - 1)), getReadableLen());
    if (n < 0) {
        *saveErrno = errno;
    }
    return n;
}
