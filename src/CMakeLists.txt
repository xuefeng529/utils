cmake_minimum_required(VERSION 2.6)

project(utils C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "debug")
endif()

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/..")
 message(STATUS ${CMAKE_INSTALL_PREFIX})

set(CXX_FLAGS
 -g
 # -DVALGRIND
 -Wall
 -Wextra
 # -m32
 -Werror
 #-Wconversion
 -Wno-deprecated
 -Wno-unused-parameter
 -Wold-style-cast
 -Woverloaded-virtual
 -Wpointer-arith
 #-Wshadow
 -Wwrite-strings
 #-march=native
 # -MMD
 # -std=c++0x
 -rdynamic
 )
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -finline-limit=1000 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

find_package(Boost REQUIRED)
include_directories(${PROJECT_SOURCE_DIR})
include_directories(/usr/local/mysql/include)
include_directories(/usr/local/include/zookeeper/)
include_directories(/usr/local/include/librdkafka/rdkafka)
#include_directories(${PROJECT_SOURCE_DIR}/http-lite)
#include_directories(${PROJECT_SOURCE_DIR}/consistent-hash)

link_directories(/usr/local/ssdb/api/cpp)
link_directories(/usr/local/mysql/lib)

add_subdirectory(base)
add_subdirectory(net)
add_subdirectory(plugins/json)
add_subdirectory(plugins/kafka)
add_subdirectory(plugins/http_lite)
add_subdirectory(plugins/consistent_hash)
add_subdirectory(plugins/ssdb_client)
add_subdirectory(plugins/mysql_client)
add_subdirectory(examples)
