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

set(c10k_SOURCE src/c10k/c10k.cpp src/c10k/c10k.hpp
        src/c10k/event_loop.hpp src/c10k/event_loop.cpp
        src/c10k/timer_fd.hpp src/c10k/queue.hpp
        src/c10k/connection.hpp src/c10k/connection.cpp
        src/c10k/worker_thread.hpp src/c10k/handler.hpp
        src/c10k/expire_record.hpp
        src/c10k/worker_thread_pool.hpp
        src/c10k/round_robin_pool.hpp src/c10k/round_robin_pool.cpp
        src/c10k/addr.hpp src/c10k/addr.cpp
        src/c10k/server_legacy.hpp
        src/c10k/utils.hpp)
add_library(c10k STATIC ${c10k_SOURCE})
target_link_libraries(c10k ${CMAKE_THREAD_LIBS_INIT})

if (c10k-build_tests)
    enable_testing()
    set(c10k_TEST_COMMON test/test_common.hpp test/handler/pingpong_handler.hpp test/worker/worker.hpp)

    add_library(catch-test-main STATIC test/test_common.hpp test/test_main.cpp)
    target_include_directories(catch-test-main PUBLIC ${Catch_INCLUDE_DIR})

    add_executable(event_loop-test test/event_loop_test.cpp ${c10k_TEST_COMMON})
    target_include_directories(event_loop-test PUBLIC ${Catch_INCLUDE_DIR})
    target_link_libraries(event_loop-test c10k catch-test-main)

    add_executable(endian-test test/endian_test.cpp ${c10k_TEST_COMMON})
    target_include_directories(endian-test PUBLIC ${Catch_INCLUDE_DIR})
    target_link_libraries(endian-test c10k catch-test-main)

    add_executable(queue-test test/queue_test.cpp ${c10k_TEST_COMMON})
    target_include_directories(queue-test PUBLIC ${Catch_INCLUDE_DIR})
    target_link_libraries(queue-test c10k catch-test-main)

    add_executable(connection-test test/connection_test.cpp ${c10k_TEST_COMMON})
    target_include_directories(connection-test PUBLIC ${Catch_INCLUDE_DIR})
    target_link_libraries(connection-test c10k catch-test-main)

    add_executable(worker_thread-test test/worker_thread_test.cpp ${c10k_TEST_COMMON})
    target_include_directories(worker_thread-test PUBLIC ${Catch_INCLUDE_DIR})
    target_link_libraries(worker_thread-test c10k catch-test-main)

    add_executable(expire_record-test test/expire_record_test.cpp ${c10k_TEST_COMMON})
    target_include_directories(expire_record-test PUBLIC ${Catch_INCLUDE_DIR})
    target_link_libraries(expire_record-test c10k catch-test-main)

    add_executable(worker_thread_pool-test test/worker_thread_pool_test.cpp ${c10k_TEST_COMMON})
    target_include_directories(worker_thread_pool-test PUBLIC ${Catch_INCLUDE_DIR})
    target_link_libraries(worker_thread_pool-test c10k catch-test-main)

    add_executable(server_legacy-test test/server_legacy_test.cpp ${c10k_TEST_COMMON})
    target_include_directories(server_legacy-test PUBLIC ${Catch_INCLUDE_DIR})
    target_link_libraries(server_legacy-test c10k catch-test-main)

    add_test(event_loop_test event_loop-test)
    add_test(endian_test endian-test)
    add_test(queue_test queue-test)
    add_test(connection_test connection-test)
    add_test(worker_thread_test worker_thread-test)
    add_test(expire_record_test expire_record-test)
    add_test(worker_thread_pool_test worker_thread_pool-test)
    add_test(server_legacy_test server_legacy-test)
endif()

