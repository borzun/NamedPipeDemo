cmake_minimum_required(VERSION 3.8)

project (NamedPipeClient)

include_directories(${CMAKE_CURRENT_LIST_DIR})

# Actually, explicitly adding header and source files is better approach than implicit cmake re-run:
# https://stackoverflow.com/questions/1027247/is-it-better-to-specify-source-files-with-glob-or-each-file-individually-in-cmak
# https://crascit.com/2016/01/31/enhanced-source-file-handling-with-target_sources/

set(CLIENT_HEADERS
	"${CMAKE_CURRENT_SOURCE_DIR}/ClassRepository.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Client.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/ClientRequest.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/ResponseParser.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/DemoSimulator.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Pipe.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/ResponseParser.h")

set(CLIENT_SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/ClassRepository.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Client.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/ClientRequest.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/ResponseParser.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/DemoSimulator.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Pipe.cpp")

# create a executable:
add_executable(NamedPipeClient ${CLIENT_HEADERS} ${CLIENT_SOURCES})
target_link_libraries(NamedPipeClient PRIVATE NamedPipeCommon)
