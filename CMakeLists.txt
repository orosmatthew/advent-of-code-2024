cmake_minimum_required(VERSION 3.25)
project(advent_of_code_2024)

set(CMAKE_CXX_STANDARD 23)

option(BENCHMARK "Enable benchmarking" OFF)
if (BENCHMARK)
    add_compile_definitions(BENCHMARK)
endif ()

if (WIN32)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -stdlib=libc++ -lc++abi")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")
    endif ()
endif ()

add_executable(day1-part1 day1-part1/main.cpp)
add_executable(day1-part2 day1-part2/main.cpp)
add_executable(day2-part1 day2-part1/main.cpp)
add_executable(day2-part2 day2-part2/main.cpp)
add_executable(day3-part1 day3-part1/main.cpp)
add_executable(day3-part2 day3-part2/main.cpp)
add_executable(day4-part1 day4-part1/main.cpp)
add_executable(day4-part2 day4-part2/main.cpp)
add_executable(day5-part1 day5-part1/main.cpp)
add_executable(day5-part2 day5-part2/main.cpp)
add_executable(day6-part1 day6-part1/main.cpp)
add_executable(day6-part2 day6-part2/main.cpp)
add_executable(day7-part1 day7-part1/main.cpp)
add_executable(day7-part2 day7-part2/main.cpp)
add_executable(day8-part1 day8-part1/main.cpp)
add_executable(day8-part2 day8-part2/main.cpp)
add_executable(day9-part1 day9-part1/main.cpp)
add_executable(day9-part2 day9-part2/main.cpp)
add_executable(day10-part1 day10-part1/main.cpp)
add_executable(day10-part2 day10-part2/main.cpp)
add_executable(day11-part1 day11-part1/main.cpp)
add_executable(day11-part2 day11-part2/main.cpp)
add_executable(day12-part1 day12-part1/main.cpp)
add_executable(day12-part2 day12-part2/main.cpp)
add_executable(day13-part1 day13-part1/main.cpp)
add_executable(day13-part2 day13-part2/main.cpp)
add_executable(day14-part1 day14-part1/main.cpp)
add_executable(day14-part2 day14-part2/main.cpp)
add_executable(day15-part1 day15-part1/main.cpp)
add_executable(day15-part2 day15-part2/main.cpp)
add_executable(day16-part1 day16-part1/main.cpp)
add_executable(day16-part2 day16-part2/main.cpp)
add_executable(day17-part1 day17-part1/main.cpp)
add_executable(day17-part2 day17-part2/main.cpp)
add_executable(day18-part1 day18-part1/main.cpp)
add_executable(day18-part2 day18-part2/main.cpp)
add_executable(day19-part1 day19-part1/main.cpp)
add_executable(day19-part2 day19-part2/main.cpp)