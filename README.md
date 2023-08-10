# Muduo网络库 C++11重构  
此项目对Muduo网络库进行C++ 11重构，去除Boost依赖，并优化其中的缓冲区。
- 重写Muduo核心组件，使用C++11重构，去依赖Boost，如使用std::function<> 替代boost::function。
- 添加定时器任务组件，选用gettimeofday + timerfd_*函数作为事件产生器，使得定时任务能够用Reactor模式管理，统一了事件源，代码一致性更好。

# 运行
```shell
git clone https://github.com/mmfoxred/mmuduo.git
cd mmuduo/mymuduo
sudo ./install.sh
# 在./build中生成libmymuduo.so
# 将头文件复制到 /usr/include/mymuduo
# 将库文件复制到 /usr/lib/mymuduo/
```
现在在项目中包含头文件即可，编译时添加 `-lmymuduo -lpthread` 参数
# 性能测试

# 性能分析
使用`pref + FlameGraph脚本` 生成On-CPU火焰图，分析其性能瓶颈。
![](./pic/perf.svg)
- 先看底部，只有系统调用 + 主功能，没有什么可优化的地方![](./pic/bottom.png)
- 继续往上看，这里有一个分叉点。![](./pic/readfd.png)分别是`可读事件的处理函数handleRead`和`readFd`读取fd内的数据到缓冲区。
    - `readFd`中使用了`readv`和`muduo独特的缓冲区put方法`。
    - put方法中根据读取的字节数决定是使用缓冲区挪移或者vector<char>的内存扩容，这确实会造成一定的开销。
- 继续往上看，这里是`Buffer::retrieveAllAsString 截取缓冲区中内容，返回string`和`send函数`。这里开销是无法避免的。![](./pic/top.png)
- 再上面就是socket的send过程，涉及Linux的网络协议栈，水平有限就不再分析了。可以看到经过传输层TCP、网络层IP。
