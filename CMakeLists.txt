cmake_minimum_required(VERSION 3.15)

set(LIB_TARGET yuv-ipp-convert)
project(${LIB_TARGET})

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(IPP)

include_directories(include)

add_library(${LIB_TARGET} SHARED
            src/YuvToRgbaConverter.cpp)
target_include_directories(${LIB_TARGET} PRIVATE ${IPP_INCLUDE_DIR} include)
target_link_libraries(${LIB_TARGET} rt pthread dl ${IPP_LIBRARIES})

add_subdirectory(tests)
