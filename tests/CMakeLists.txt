add_executable(test_timer timer_test.cpp)
target_link_libraries(test_timer mini_muduo)

add_test(
    NAME test_timer
    COMMAND test_timer
)