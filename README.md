# mini_muduo

`mini_muduo` 是一个基于 C++11 编写的高性能网络库，旨在通过复刻 [muduo](https://github.com/chenshuo/muduo) 的核心架构来深入学习网络编程。

## 🌟 核心特性

- **现代 C++ 风格**：基于 C++11，使用 RAII、`shared_ptr` 和 `unique_ptr` 管理核心资源。
- **Reactor 模式**：采用主流的 `Main-Sub Reactor` 架构（多 Reactor + 多 IO 线程）。
- **非阻塞 IO + Epoll**：底层使用 `epoll` (LT 模式) 进行多路复用。
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
    - `TimingWheel`: 连接空闲超时管理
    - `TimerQueue`: 定时任务管理
- `include/mini_muduo/base`：线程、时间戳、异常和日志等基础组件
- `src`：实现代码
- `examples`：Echo Server 示例
- `tests`：日志与定时器测试

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
