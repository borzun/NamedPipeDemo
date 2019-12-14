cmake_minimum_required(VERSION 3.8)

project (NamedPipeCommon)

# Actually, explicitly adding header and source files is better approach than implicit cmake re-run:
# https://stackoverflow.com/questions/1027247/is-it-better-to-specify-source-files-with-glob-or-each-file-individually-in-cmak
# https://crascit.com/2016/01/31/enhanced-source-file-handling-with-target_sources/

set(COMMON_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/CustomClass.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/DataSerializer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/DataDeserializer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Logger.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Types.h"
    )

set(COMMON_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/CustomClass.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/DataDeserializer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/DataSerializer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Logger.cpp")

# create a library:
add_library(NamedPipeCommon STATIC ${COMMON_HEADERS} ${COMMON_SOURCES})
target_include_directories(NamedPipeCommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
