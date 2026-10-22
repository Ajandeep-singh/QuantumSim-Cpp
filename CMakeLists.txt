cmake_minimum_required(VERSION 3.16)
project(QuantumSimCpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(quantumsim_core
    src/StateVector.cpp
    src/Gates.cpp
    src/Circuit.cpp
    src/Measurement.cpp
)
target_include_directories(quantumsim_core PUBLIC include)

add_executable(quantumsim src/main.cpp)
target_link_libraries(quantumsim PRIVATE quantumsim_core)

add_executable(phase1_demo src/phase1_demo.cpp)
target_link_libraries(phase1_demo PRIVATE quantumsim_core)

add_executable(quantumsim_tests tests/test_quantumsim.cpp)
target_link_libraries(quantumsim_tests PRIVATE quantumsim_core)

add_executable(quantumsim_benchmarks benchmarks/benchmark.cpp)
target_link_libraries(quantumsim_benchmarks PRIVATE quantumsim_core)

foreach(target quantumsim_core quantumsim phase1_demo quantumsim_tests quantumsim_benchmarks)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

enable_testing()
add_test(NAME quantumsim_validation COMMAND quantumsim_tests)
