cmake_minimum_required(VERSION 3.10)
project(GameServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

#找到系统里安装的Protobuf
find_package(Protobuf REQUIRED)

include_directories(
	${PROJECT_SOURCE_DIR}/src
	${Protobuf_INCLUDE_DIRS}
)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.cc")

add_executable(game_server ${SOURCES})

#链接Protobuf库
target_link_libraries(game_server ${Protobuf_LIBRARIES})
