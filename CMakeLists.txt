cmake_minimum_required(VERSION 2.6)

project(slack CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CXX_FLAGS 
    -g 
    -Wall 
    -Werror 
    -Wextra
    -Wno-unused-parameter
    -Wold-style-cast
    -Woverloaded-virtual
    -Wpointer-arith
    -Wshadow
    -Wwrite-strings
    -std=c++11 
    -rdynamic)

string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")
set(CMAKE_CXX_COMPILER "g++")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

#find_package(Boost REQUIRED)

#include_directories(${BOOST_INCLUDE_DIRS})

include_directories(${PROJECT_SOURCE_DIR})
message(STATUS "Source_dir:" ${PROJECT_SOURCE_DIR})

add_subdirectory(src/base)
#add_subdirectory(src/net)
#add_subdirectory(tests)