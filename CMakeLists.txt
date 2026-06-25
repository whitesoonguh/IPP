cmake_minimum_required(VERSION 3.16)

project(IPP
        VERSION 1.0
        DESCRIPTION "Inner Pairing Product"
        LANGUAGES CXX ASM)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)        

add_compile_options(-O2 -march=native)

# Settings for MCL
set(MCL_USE_GMP       OFF CACHE BOOL "" FORCE)
set(MCL_USE_LLVM      OFF CACHE BOOL "" FORCE)
set(MCL_BUILD_SAMPLE  OFF CACHE BOOL "" FORCE)
set(MCL_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(MCL_TEST_WITH_GMP OFF CACHE BOOL "" FORCE)
add_subdirectory(mcl)


set(IPPSource src/core.cpp src/main.cpp)

add_executable(ipptest ${IPPSource})
set_target_properties(ipptest PROPERTIES OUTPUT_NAME test)

target_include_directories(ipptest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ipptest PRIVATE
    mcl::mcl_st
)
