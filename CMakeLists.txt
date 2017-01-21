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

set(c10k_SOURCE src/c10k/c10k.cpp)
add_library(c10k STATIC ${c10k_SOURCE})

if (c10k-build_tests)
    enable_testing()

    add_executable(c10k-test test/c10k_test.cpp test/test_main.cpp)
    target_include_directories(c10k-test PUBLIC ${Catch_INCLUDE_DIR})
    target_link_libraries(c10k-test c10k)

    add_test(c10k_test c10k-test)
endif()

