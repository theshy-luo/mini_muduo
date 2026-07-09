# test_timer
add_executable(test_timer timer_test.cpp)
target_link_libraries(test_timer mini_muduo)

add_test(
    NAME test_timer
    COMMAND test_timer)

# test_logger
add_executable(test_logger Log_test.cc)
target_link_libraries(test_logger mini_muduo)

add_test(
    NAME test_logger
    COMMAND test_logger)

# test_unique_fd
add_executable(test_unique_fd test_unique_fd.cc)
target_link_libraries(test_unique_fd mini_muduo)

add_test(
    NAME test_unique_fd
    COMMAND test_unique_fd)

# test_udp_socket
add_executable(test_udp_socket test_udp_socket.cc)
target_link_libraries(test_udp_socket mini_muduo)

add_test(
    NAME test_udp_socket
    COMMAND test_udp_socket)

# test_udp_receiver
add_executable(test_udp_receiver test_udp_receiver.cc)
target_link_libraries(test_udp_receiver mini_muduo)

add_test(
    NAME test_udp_receiver
    COMMAND test_udp_receiver)

# test_packet_buffer_pool
add_executable(test_packet_buffer_pool test_packet_buffer_pool.cc)
target_link_libraries(test_packet_buffer_pool mini_muduo)

add_test(
    NAME test_packet_buffer_pool
    COMMAND test_packet_buffer_pool)