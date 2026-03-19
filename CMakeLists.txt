cmake_minimum_required(VERSION 3.10)
project(mini_muduo CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 让 CMake 输出 compile_commands.json 给 clangd 使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 编译选项
add_compile_options(-Wall -Wextra -g)

# 开启 ASan
add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
link_libraries(asan)

enable_testing()
# 暂时强制只加入当前工程的test
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

# 查找线程库
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# 设置全局头文件搜索路径，使得外部可以使用 #include <mini_muduo/xxx.h>
include_directories(${PROJECT_SOURCE_DIR}/include)

# 提供一个配置选项来选择生成静态库还是动态库 (默认生成静态库)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# 编译生成核心静态/动态库 (如果 BUILD_SHARED_LIBS 为 ON 就是动态库，否则就是静态库)
file(GLOB CORE_SOURCES "src/*.cpp" "src/*.cc" "src/base/*.cpp" "src/base/*.cc")
add_library(mini_muduo ${CORE_SOURCES})
target_link_libraries(mini_muduo PUBLIC Threads::Threads)

# 添加子目录
add_subdirectory("examples/echo_server")
add_subdirectory("tests")
