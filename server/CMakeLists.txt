cmake_minimum_required(VERSION 3.8)

project (NamedPipeServer)

include_directories(${CMAKE_CURRENT_LIST_DIR})

# Actually, explicitly adding header and source files is better approach than implicit cmake re-run:
# https://stackoverflow.com/questions/1027247/is-it-better-to-specify-source-files-with-glob-or-each-file-individually-in-cmak
# https://crascit.com/2016/01/31/enhanced-source-file-handling-with-target_sources/

set(SERVER_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/RequestParser.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ClassRegistry.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/CustomClassParser.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/PipeInstance.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Server.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ServerResponse.h"

)

set(SERVER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CustomClassParser.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/RequestParser.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/PipeInstance.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Server.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ServerResponse.cpp"
)

# create a executable:
add_executable(NamedPipeServer ${SERVER_HEADERS} ${SERVER_SOURCES})

target_link_libraries(NamedPipeServer PRIVATE NamedPipeCommon)
