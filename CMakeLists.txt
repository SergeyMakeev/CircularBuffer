cmake_minimum_required(VERSION 3.14)

project(CircularBuffer 
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "High-performance circular buffer implementation"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default build type for performance
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Include GoogleTest and Google Benchmark
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
)

FetchContent_Declare(
    googlebenchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest googlebenchmark)
enable_testing()
include(GoogleTest)

# Add the circular_buffer subdirectory first
add_subdirectory("${PROJECT_SOURCE_DIR}/circular_buffer")

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Test source files
set(TEST_SOURCES
    tests/circular_buffer_test.cpp
    tests/iterator_debug_test.cpp
)

# Add circular buffer test executable
add_executable(circular_buffer_tests ${TEST_SOURCES})

# Link Google Test and circular_buffer
target_link_libraries(circular_buffer_tests 
    gtest_main
    gmock_main
    dod::circular_buffer
)

# Add performance test executable
add_executable(performance_test
    tests/performance_test.cpp
)

# Link Google Benchmark and circular_buffer library to performance test
target_link_libraries(performance_test 
    benchmark::benchmark
    dod::circular_buffer
)

# Set Visual Studio startup project
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT circular_buffer_tests)

# Compiler-specific options
if(MSVC)
    target_compile_options(circular_buffer_tests PRIVATE /W4 /WX /permissive- /EHsc)
    target_compile_options(performance_test PRIVATE /W4 /EHsc)
    
    # Enable iterator debugging in debug builds
    target_compile_definitions(circular_buffer_tests PRIVATE 
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=2>
    )
else()
    target_compile_options(circular_buffer_tests PRIVATE -Wall -Wextra -pedantic -Werror)
    target_compile_options(performance_test PRIVATE -Wall -Wextra -pedantic)
    
    # Enable debug flags for debug builds
    target_compile_options(circular_buffer_tests PRIVATE 
        $<$<CONFIG:Debug>:-g -O0 -DDEBUG>
    )
endif()

# Register tests with CTest
gtest_discover_tests(circular_buffer_tests)

# Optional: Add address sanitizer support
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(ENABLE_ASAN AND NOT MSVC)
    target_compile_options(circular_buffer_tests PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(circular_buffer_tests PRIVATE -fsanitize=address)
    
    target_compile_options(performance_test PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(performance_test PRIVATE -fsanitize=address)
endif()

# Optional: Add thread sanitizer support
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
if(ENABLE_TSAN AND NOT MSVC)
    target_compile_options(circular_buffer_tests PRIVATE -fsanitize=thread)
    target_link_options(circular_buffer_tests PRIVATE -fsanitize=thread)
endif()

# Optional: Add memory sanitizer support  
option(ENABLE_MSAN "Enable MemorySanitizer" OFF)
if(ENABLE_MSAN AND NOT MSVC)
    target_compile_options(circular_buffer_tests PRIVATE -fsanitize=memory -fno-omit-frame-pointer)
    target_link_options(circular_buffer_tests PRIVATE -fsanitize=memory)
endif()

# Coverage support
option(ENABLE_COVERAGE "Enable code coverage" OFF)
if(ENABLE_COVERAGE AND NOT MSVC)
    target_compile_options(circular_buffer_tests PRIVATE --coverage -O0)
    target_link_options(circular_buffer_tests PRIVATE --coverage)
endif()

# Print configuration info
message(STATUS "CircularBuffer Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Address Sanitizer: ${ENABLE_ASAN}")
message(STATUS "  Thread Sanitizer: ${ENABLE_TSAN}")
message(STATUS "  Memory Sanitizer: ${ENABLE_MSAN}")
message(STATUS "  Coverage: ${ENABLE_COVERAGE}") 