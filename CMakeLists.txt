cmake_minimum_required(VERSION 3.1)
project(c10k-server)

set(c10k-build_tests OFF CACHE BOOL "Whether to build tests")

# Enable C++14
set (CMAKE_CXX_STANDARD 14)

################################
# Find thread
################################
find_package(Threads REQUIRED)

################################
# Catch
################################
set(Catch_INCLUDE_DIR vendor/Catch/include)

##################################
# Spdlog
##################################
set(spdlog_SOURCE_DIR ${c10k-server_SOURCE_DIR}/vendor/spdlog)
include_directories(${spdlog_SOURCE_DIR}/include)


##################################
include_directories(src)

set(c10k_SOURCE src/c10k/c10k.cpp src/c10k/c10k.hpp src/c10k/event_loop.hpp src/c10k/event_loop.cpp src/c10k/timer_fd.hpp)
add_library(c10k STATIC ${c10k_SOURCE})
target_link_libraries(c10k ${CMAKE_THREAD_LIBS_INIT})

if (c10k-build_tests)
    enable_testing()

    add_executable(event_loop-test test/event_loop_test.cpp test/test_main.cpp)
    target_include_directories(event_loop-test PUBLIC ${Catch_INCLUDE_DIR})
    target_link_libraries(event_loop-test c10k)

    add_executable(endian-test test/endian_test.cpp test/test_main.cpp)
    target_include_directories(endian-test PUBLIC ${Catch_INCLUDE_DIR})
    target_link_libraries(endian-test c10k)

    add_test(event_loop_test event_loop-test)
    add_test(endian_test endian-test)
endif()

