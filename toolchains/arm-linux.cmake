# 交叉编译工具链示例 (Arm)
# cmake -DCMAKE_TOOLCHAIN_FILE=../toolchains/arm-linux.cmake ..
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 根据你的实际交叉编译工具链路径进行修改
set(CMAKE_C_COMPILER /usr/bin/toolchain/V851S/toolchain/bin/arm-openwrt-linux-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/toolchain/V851S/toolchain/bin/arm-openwrt-linux-g++)

# 关闭 asan
set(ENABLE_ASAN OFF CACHE BOOL "" FORCE)
