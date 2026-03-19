add_executable(simple_echo_sever simple_echo_sever.cpp)
add_executable(EchoServer EchoServer.cpp)

# 链接核心网络库 mini_muduo
target_link_libraries(simple_echo_sever mini_muduo)
target_link_libraries(EchoServer mini_muduo)
