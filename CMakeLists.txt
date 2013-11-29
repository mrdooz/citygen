# export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig

cmake_minimum_required(VERSION 2.8)
project(citygen)

set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include_directories(/opt/local/include/boost)

include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)
include_directories(${SDL2_INCLUDE_DIRS})

#installed via macports => /opt/local/include
include_directories(${GLEW_INCLUDE_DIR})

find_package(OpenGL)
find_package(GLEW)

add_executable(citygen citygen.cpp file_utils.cpp imgui.cpp imguiRenderGL.cpp)
target_link_libraries(citygen ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES})