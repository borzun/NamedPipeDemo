cmake_minimum_required(VERSION 3.8)

project(NamedPipeDemo)

set (CMAKE_CXX_STANDARD 17)

# Is this required???
# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules/cmake/")

add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)
