#include "TcpConnection.h"
#include "Callbacks.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Socket.h"

#include <errno.h>
#include <netinet/tcp.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <functional>
#include <string>

static EventLoop* CheckLoopNotNull(EventLoop* loop) {
    if (loop == nullptr) {
        LOG_FATAL("%s:%s:%d TcpConnection Loop is null! \n", __FILE__,
                  __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop, const std::string& nameArg,
                             int sockfd, const InetAddress& localAddr,
                             const InetAddress& peerAddr)
    : m_eventLoop(CheckLoopNotNull(loop)),
      m_name(nameArg),
      m_state(kConnecting),
      m_isReading(true),
      m_socket(new Socket(sockfd)),
      m_channel(new Channel(loop, sockfd)),
      m_localAddr(localAddr),
      m_peerAddr(peerAddr),
      m_highWaterMark(64 * 1024 * 1024)  // 64M
{
    m_inputBuffer.initBuffer();
    m_outputBuffer.initBuffer();
    // 下面给channel设置相应的回调函数，poller给channel通知感兴趣的事件发生了，channel会回调相应的操作函数
    m_channel->setReadCallBack(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    m_channel->setWriteCallBack(std::bind(&TcpConnection::handleWrite, this));
    m_channel->setCloseCallBack(std::bind(&TcpConnection::handleClose, this));
    m_channel->setErrorCallBack(std::bind(&TcpConnection::handleError, this));

    //LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", m_name.c_str(), sockfd);
    m_socket->set_keepAlive(true);
}

TcpConnection::~TcpConnection() {
    // LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n", m_name.c_str(),
    //          m_channel->get_fd(), (int)m_state);
}

//实际调用的是TcpConnection::sendInLoop
void TcpConnection::send(const std::string& buf) {
    if (m_state == kConnected) {
        if (m_eventLoop->isInLoopThread()) {
            sendInLoop(buf.c_str(), buf.size());
        } else {
            m_eventLoop->runInLoop(std::bind(&TcpConnection::sendInLoop, this,
                                             buf.c_str(), buf.size()));
        }
    }
}

/**
 * 发送数据  应用写的快， 而内核发送数据慢， 需要把待发送数据写入缓冲区， 而且设置了水位回调
 */
void TcpConnection::sendInLoop(const void* data, size_t len) {
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    // 之前调用过该connection的shutdown，不能再进行发送了
    if (m_state == kDisconnected) {
        LOG_ERROR("disconnected, give up writing!");
        return;
    }

    // 表示m_channel第一次开始写数据，而且缓冲区没有待发送数据
    if (!m_channel->isWriting() && m_outputBuffer.getReadableLen() == 0) {
        nwrote = ::write(m_channel->get_fd(), data, len);
        if (nwrote >= 0) {
            remaining = len - nwrote;
            if (remaining == 0 && m_writeCompleteCallback) {
                // 既然在这里数据全部发送完成，就不用再给channel设置epollout事件了
                m_eventLoop->queueInLoop(
                    std::bind(m_writeCompleteCallback, shared_from_this()));
            }
        } else  // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET)  // SIGPIPE  RESET
                {
                    faultError = true;
                }
            }
        }
    }

    // 说明当前这一次write，并没有把数据全部发送出去，剩余的数据需要保存到缓冲区当中，然后给channel
    // 注册epollout事件，poller发现tcp的发送缓冲区有空间，会通知相应的sock-channel，调用writeCallback_回调方法
    // 也就是调用TcpConnection::handleWrite方法，把发送缓冲区中的数据全部发送完成
    if (!faultError && remaining > 0) {
        // 目前发送缓冲区剩余的待发送数据的长度
        size_t oldLen = m_outputBuffer.getReadableLen();
        //触发了高水位情况
        if (oldLen + remaining >= m_highWaterMark && oldLen < m_highWaterMark &&
            m_highWaterMarkCallback) {
            m_eventLoop->queueInLoop(std::bind(m_highWaterMarkCallback,
                                               shared_from_this(),
                                               oldLen + remaining));
        }
        m_outputBuffer.writeBuffer((char*)data + nwrote, remaining);
        if (!m_channel->isWriting()) {
            m_channel
                ->enableWriting();  // 这里一定要注册channel的写事件，否则poller不会给channel通知epollout
        }
    }
}

// 关闭连接，都是关闭写端来关闭连接的
void TcpConnection::shutdown() {
    if (m_state == kConnected) {
        setState(kDisconnecting);
        m_eventLoop->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop() {
    if (!m_channel->isWriting())  // 说明outputBuffer中的数据已经全部发送完成
    {
        m_socket->shutdownWrite();  // 关闭写端
    }
}

// 连接建立
//执行 Channel::tie() + Channel::enableReading 注册EPOLLIN事件 + 执行m_connectionCallback外部传入的回调
void TcpConnection::connectEstablished() {
    setState(kConnected);
    m_channel->tie(shared_from_this());
    m_channel->enableReading();  // 向poller注册channel的EPOLLIN事件

    // 新连接建立，执行回调
    m_connectionCallback(shared_from_this());
}

// 连接销毁
void TcpConnection::connectDestroyed() {
    if (m_state == kConnected) {
        setState(kDisconnected);
        m_channel
            ->disableAll();  // 把channel的所有感兴趣的事件，从poller中del掉
        m_connectionCallback(shared_from_this());
    }
    m_channel->remove();  // 把channel从poller中删除掉
}

/*
传给Channel的可读回调事件
当Poller->poll()返回时，收到EPOLLIN事件，就会调用Channel执行该回调函数
作用：
	1.读取可读事件中fd发送的数据
	2.调用ChatServer::m_messageCallback() 对这些数据进行处理
	3.recv的各种判断处理 retSize == n等情况
*/
void TcpConnection::handleRead(Timestamp receiveTime) {
    int savedErrno = 0;
    ssize_t n = m_inputBuffer.readFd(m_channel->get_fd(), &savedErrno);
    if (n > 0) {
        // 已建立连接的用户，有可读事件发生了，调用用户传入的回调操作onMessage
        m_messageCallback(shared_from_this(), &m_inputBuffer, receiveTime);
    } else if (n == 0) {
        handleClose();
    } else {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite() {
    if (m_channel->isWriting()) {
        int savedErrno = 0;
        //写完整个readableBytes
        ssize_t n = m_outputBuffer.writeFd(m_channel->get_fd(), &savedErrno);
        if (n > 0) {
            //置位readIndex,handleRead()中也有置位操作，但是封装在readFd()中
            // m_outputBuffer.retrieve(static_cast<size_t>(n));

            //外部可读大小为0，表示读完了
            if (m_outputBuffer.getReadableLen() == 0) {
                m_channel->disableWriting();  //那么就取消写事件
                //如果注册了写完成事件，还可以处理
                if (m_writeCompleteCallback) {
                    // 唤醒m_eventLoop对应的thread线程，执行回调
                    m_eventLoop->queueInLoop(
                        std::bind(m_writeCompleteCallback, shared_from_this()));
                }
                if (m_state == kDisconnecting) {
                    shutdownInLoop();
                }
            }
        } else {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    } else {
        LOG_ERROR("TcpConnection fd=%d is down, no more writing \n",
                  m_channel->get_fd());
    }
}

void TcpConnection::handleClose() {
    // LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n",
    //          m_channel->get_fd(), (int)m_state);
    setState(kDisconnected);
    m_channel->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    m_connectionCallback(connPtr);  // 执行连接关闭的回调
    m_closeCallback(
        connPtr);  // 关闭连接的回调  执行的是TcpServer::removeConnection回调方法
}

void TcpConnection::handleError() {
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(m_channel->get_fd(), SOL_SOCKET, SO_ERROR, &optval,
                     &optlen) < 0) {
        err = errno;
    } else {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n",
              m_name.c_str(), err);
}