# mini_muduo

`mini_muduo` 是一个基于 C++11 编写的高性能网络库，旨在通过复刻 [muduo](https://github.com/chenshuo/muduo) 的核心架构来深入学习网络编程。

## 🌟 核心特性

- **现代 C++ 风格**：基于 C++11，使用 RAII、`shared_ptr` 和 `unique_ptr` 管理核心资源。
- **Reactor 模式**：采用主流的 `Main-Sub Reactor` 架构（多 Reactor + 多 IO 线程）。
- **非阻塞 IO + Epoll**：底层使用 `epoll` (LT 模式) 进行多路复用。
- **批量 UDP 接收器**：基于 Linux `recvmmsg` 实现批量收包，配合 `PacketBufferPool` 复用接收缓冲区。
- **用户态零额外拷贝传递**：UDP payload 写入 `PacketBuffer` 后，通过只读 `UdpDatagram` 句柄在业务层共享，避免 callback 外缓存时再次复制 payload。
- **定时器队列**：基于 `timerfd` 与 `std::set` 实现的高效定时任务调度。
- **连接空闲检测**：基于环形时间轮自动关闭超时连接。
- **优雅关闭 (Graceful Shutdown)**：支持对已标记关闭连接的输出缓冲区排空，确保数据不丢失。
- **统一日志系统**：支持 `TRACE`、`DEBUG`、`INFO`、`WARN`、`ERROR` 和 `FATAL` 六个级别。
- **健壮性**：默认支持 ASan (Address Sanitizer)，并提供并发 Echo 测试脚本。

## 🛠️ 快速开始

### 依赖环境

- Linux (内核 2.6.28+)
- CMake (3.10+)
- GCC/Clang (支持 C++11)
- Python 3（使用构建或并发测试脚本时需要）

### 编译运行

```bash
# 1. 使用构建脚本 (推荐)
./build.py --cxx clang++              # 指定编译器构建
./build.py --type Release             # 构建 Release 版本
./build.py --clean                    # 仅清理 build 目录
./build.py --clean --type Debug       # 清理后重新构建

# 2. 传统方式 (CMake)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4

# 3. 运行示例（Echo Server 带超时检测）
./build/examples/echo_server/EchoServerWithTimeout
```

ASan 默认开启。如需关闭：

```bash
cmake -S . -B build -DENABLE_ASAN=OFF
cmake --build build -j4
```

## 📂 模块导览

- `include/mini_muduo`：库核心头文件
    - `TcpServer`: 核心服务类
    - `EventLoop`: 事件循环与任务队列
    - `UdpSocket`: 非阻塞 UDP socket 封装
    - `UdpReceiver`: 基于 `recvmmsg` 的批量 UDP 接收器
    - `UdpDatagram`: 只读 UDP 数据报句柄
    - `PacketBuffer` / `PacketBufferPool`: UDP payload 缓冲区与缓冲池
    - `TimingWheel`: 连接空闲超时管理
    - `TimerQueue`: 定时任务管理
- `include/mini_muduo/base`：线程、时间戳、异常、日志和 `UniqueFd` 等基础组件
- `src`：实现代码
- `examples`：Echo Server 示例
- `tests`：日志、定时器、UDP socket、UDP receiver 和缓冲池测试

## 📡 UDP Receiver

当前 UDP 接收器是 Linux 专用实现，核心目标是先跑通高性能 UDP 接收的底层模型：

```text
UDP socket
  -> recvmmsg 批量接收
  -> PacketBufferPool 获取可写缓冲区
  -> UdpDatagram 只读共享句柄
  -> message callback 按值接收并可长期保存
```

主要组件：

- `UniqueFd`：RAII 管理 fd 生命周期，避免手动 close 遗漏。
- `UdpSocket`：创建 `SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC` UDP socket，并封装 bind / socket option。
- `PacketBuffer`：保存单个 UDP payload，内部使用 `std::uint8_t[]` 表达二进制数据。
- `PacketBufferPool`：预分配并复用 `PacketBuffer`，降低热路径频繁分配开销。
- `UdpDatagram`：业务侧只读的数据报句柄，内部持有共享 `PacketBuffer`，callback 外保存时不会复制 payload。
- `UdpReceiver`：通过 `recvmmsg` 一次系统调用批量接收多个 UDP datagram，并通过 `max_packets_per_read` 限制单次读事件处理量，避免长时间占用 EventLoop。

注意：

- 这里的“零额外拷贝”指用户态业务传递层面：payload 从 socket 读取到 `PacketBuffer` 后，业务缓存 `UdpDatagram` 不再复制 payload。
- 当前不是内核态到用户态的真正 zero-copy，`recvmmsg` 仍会把数据从内核 socket receive queue 拷贝到用户态 buffer。
- UDP 保留 datagram 边界。如果接收 buffer 小于 datagram 原始长度，数据会被截断，剩余部分会被内核丢弃。业务应检查 `UdpDatagram::truncated()`。
- 推荐 sender 在应用层控制单个 UDP payload 大小，例如按 MTU 约束在 1200 字节左右；大消息应在应用层分片和重组。

## 📝 日志系统

引入 `Logger.h` 后可直接使用流式日志宏。每条日志会在当前语句结束、临时 `Logger` 析构时输出：

```cpp
#include "mini_muduo/base/Logger.h"

LOG_DEBUG << "connection established, fd=" << fd;
LOG_INFO << "server started";
LOG_ERROR << "read failed, errno=" << errno;
```

`LOG_FATAL` 会先按原有格式输出并刷新日志，然后抛出 `mini_muduo::Exception`，调用方可以在测试或业务边界捕获：

```cpp
try
{
    LOG_FATAL << "unrecoverable error";
}
catch (const mini_muduo::Exception& ex)
{
    LOG_ERROR << "caught fatal exception: " << ex.what();
}
```

不要在另一个异常正在进行栈展开时调用 `LOG_FATAL`，否则析构期间再次抛出异常会触发 `std::terminate()`。当前每条日志使用 4000 字节固定缓冲区，不会自动扩容，应避免把大块数据写入单条日志。

## 🧪 测试验证

运行 CTest：

```bash
ctest --test-dir build --output-on-failure
```

也可以单独运行关键测试：

```bash
./build/tests/test_unique_fd
./build/tests/test_udp_socket
./build/tests/test_packet_buffer_pool
./build/tests/test_udp_receiver
```

UDP receiver 测试覆盖：

- 普通 UDP datagram 接收；
- callback 外保存 `UdpDatagram` 后仍可读取 payload；
- 接收 buffer 小于 datagram 时的截断标记和原始长度；
- 0 字节 UDP datagram；
- 多个 datagram 批量接收路径。

运行并发 Echo 测试前，先启动监听 `20000` 端口的服务器：

```bash
# 终端 1
./build/examples/echo_server/EchoServerWithTimeout

# 终端 2
python3 test_clinet.py
```

`test_clinet.py` 默认创建 10000 个客户端线程，运行前请根据机器资源调整脚本末尾的 `range` 数量。

## 🎨 代码格式

项目根目录提供 `.clang-format`，C/C++ 代码统一使用 4 个空格缩进且不使用 Tab。安装 `clang-format` 后可格式化单个文件：

```bash
clang-format -i src/Channel.cc
```

格式化全部源码：

```bash
find include src examples tests -type f \
  \( -name '*.h' -o -name '*.hpp' -o -name '*.cc' -o -name '*.cpp' \) \
  -exec clang-format -i {} +
```

---
*本项目仅供交流学习使用。*
