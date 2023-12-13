cmake_minimum_required(VERSION 3.24)
project(test)

set(CMAKE_CXX_STANDARD 20)

aux_source_directory(./source SRC_DIR)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp -std=gnu++20 -Werror -W -O2 -ftemplate-depth=4096 -lgomp -lpthread -fmodules-ts")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp -Wall -W -O2")

set(BOOST_DEBUG ON)

set(fmt_DIR "sublib/lib/fmt")
set(Boost_DIR "sublib/lib/")

find_package(fmt REQUIRED)
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${BOOST_LIBRARY_DIRS})
add_definitions(-DBOOST_ALL_DYN_LINK)

add_executable(test ${SRC_DIR})
target_link_libraries(test ${Boost_LIBRARIES})
target_link_libraries(test fmt)