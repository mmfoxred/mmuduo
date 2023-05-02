#pragma once

#include <cstddef>
#include <functional>
#include <memory>

class Buffer;
class TcpConnection;
class Timestamp;
class EventLoop;

using ThreadInitCallback = std::function<void(EventLoop*)>;
using ConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const ConnectionPtr&)>;
using CloseCallback = std::function<void(const ConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const ConnectionPtr&)>;
using MessageCallback =
    std::function<void(const ConnectionPtr&, Buffer*, Timestamp)>;

using HighWaterMarkCallback =
    std::function<void(const ConnectionPtr&, std::size_t)>;