# Muduo网络库 C++11重构  
此项目对Muduo网络库进行C++ 11重构，去除Boost依赖，并优化其中的缓冲区。
- 重写Muduo核心组件，使用C++11重构，去依赖Boost，如使用std::function<> 替代boost::function。
- 添加定时器任务组件，选用gettimeofday + timerfd_*函数作为事件产生器，使得定时任务能够用Reactor模式管理，统一了事件源，代码一致性更好。

# 运行
```shell

```
# 性能分析
同样

# 性能测试
