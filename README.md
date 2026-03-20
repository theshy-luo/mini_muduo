# mini_muduo

`mini_muduo` 是一个基于 C++11 编写的高性能网络库，旨在通过复刻 [muduo](https://github.com/chenshuo/muduo) 的核心架构来深入学习网络编程。

## 🌟 核心特性

- **现代 C++ 风格**：完全基于 C++11 标准，使用 `shared_ptr` / `unique_ptr` 管理资源，避免裸指针。
- **Reactor 模式**：采用主流的 `Main-Sub Reactor` 架构（多 Reactor + 多 IO 线程）。
- **非阻塞 IO + Epoll**：底层使用 `epoll` (LT 模式) 进行多路复用。
- **定时器队列**：基于 `timerfd` 与 `std::set` 实现的高效定时任务调度。
- **高性能时间轮 (Timing Wheel)**：集成了无锁分层时间轮，支持数万并发连接的 idle 超时自动清理。
- **优雅关闭 (Graceful Shutdown)**：支持对已标记关闭连接的输出缓冲区排空，确保数据不丢失。
-  **健壮性**：内置 ASan (Address Sanitizer) 支持，并通过了 9000+ 并发连接的内存压力测试。

## 🛠️ 快速开始

### 依赖环境
- Linux (内核 2.6.28+)
- CMake (3.10+)
- GCC/Clang (支持 C++11)

### 编译运行
```bash
# 1. 使用构建脚本 (推荐)
python3 build.py --cxx clang++        # 指定编译器构建
python3 build.py --type Release       # 构建 Release 版本
python3 build.py --clean              # 清理并重新构建

# 2. 传统方式 (CMake)
mkdir build && cd build
cmake ..
make -j4

# 3. 运行示例（Echo Server 带超时检测）
./build/examples/echo_server/EchoServerWithTimeout
```

## 📂 模块导览

- `/include/mini_muduo`: 库核心头文件
    - `TcpServer`: 核心服务类
    - `EventLoop`: 事件循环与任务队列
    - `TimingWheel`: 基于分格的时间轮
    - `TimerQueue`: 定时任务管理
- `/src`: 实现代码
- `/examples`: 丰富的学习例程
- `/tests`: 单元测试

## 🧪 测试验证
我们提供了 Python 压测脚本，可在 `build` 目录下运行：

```bash
# 运行超时测试
python3 ../test_timeout.py

# 运行高并发 Echo 测试
python3 ../test_clinet.py
```

---
*本项目仅供交流学习使用。*
