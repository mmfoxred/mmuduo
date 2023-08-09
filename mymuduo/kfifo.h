#pragma once

#include <mutex>
#include <memory>

//判断x是否是2的次方
#define is_power_of_two(x) ((x) != 0 && (((x) & ((x) - 1)) == 0))

//取a和b中最小值
#define Min(a, b) (((a) < (b)) ? (a) : (b))

//环形缓存
class Kfifo
{
public:
    Kfifo();
    ~Kfifo();

    //根据传入size 初始化缓冲区
    bool initBuffer(uint32_t bufferSize = 4096);
    //释放缓冲区
    void freeBuffer();
    //重置缓冲区（不需要重新申请内存）
    void resetBuffer();

    //缓存区是否为空
    bool isEmpty();
    //缓存区是否已满
    bool isFull();
    //返回可读数据长度
    uint32_t getReadableLen();
    //返回剩余空闲长度
    uint32_t getRemainLen();
    //返回缓冲区总长度
    uint32_t getBufferSize();

    //缓存区写入数据, 返回实际写入大小
    uint32_t writeBuffer(char *inBuf, uint32_t inSize);
    //缓存区读出数据 返回实际读出大小
    uint32_t readBuffer(char *outBuf, uint32_t outSize);

    ssize_t readFd(int fd, int* saveErrno);
    ssize_t writeFd(int fd, int* saveErrno);
    std::string retrieveAllAsString();

private:
    uint8_t* buffer;        //缓冲区
    uint32_t bufferSize;    //缓冲区总大小
    uint32_t write;         //写入位置
    uint32_t read;          //读出位置
    std::mutex mutex;       //互斥锁
};