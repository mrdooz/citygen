cmake_minimum_required (VERSION 2.6)

project (citygen)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules" ${CMAKE_MODULE_PATH})
# todo, why is this required to be upper case?
find_package(BRISTOL COMPONENTS main)
find_package(Protobuf REQUIRED)

include_directories(${BRISTOL_INCLUDE_DIR})
include_directories(${PROTOBUF_INCLUDE_DIR})

file(GLOB SRC "*.cpp" "*.hpp" "protocol/*" "imgui/*")

add_executable(${PROJECT_NAME} ${SRC})

if (APPLE)
  # change c++ standard library to libc++ (llvm)
  set(COMMON_FLAGS "-Wno-switch-enum")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v -std=c++11 -stdlib=libc++")
  find_library(APP_SERVICES ApplicationServices)
  find_library(OPEN_GL OpenGL)
  find_library(COCOA Cocoa)
  find_library(IOKIT IOKit)
  find_library(CORE_VIDEO CoreVideo)

  set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    XCODE_ATTRIBUTE_GCC_PREFIX_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/precompiled.hpp"
    XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER "YES"
    LINK_FLAGS "-F/Library/Frameworks")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")

  # specifically link against a protobuf and boost build with libc++
  target_link_libraries(${PROJECT_NAME}
    "/Users/dooz/Library/Caches/appCode30/DerivedData/bristol-e7802bc5/Build/Products/Debug/libbristol_main.a"
    "/usr/local/Cellar/glew/1.10.0/lib/libGLEW.a"
    "/usr/local/lib/libglfw3.a"
    "/opt/local/protobuf/lib/libprotobuf.a"
    ${OPEN_GL}
    ${COCOA}
    ${IOKIT}
    ${CORE_VIDEO}
    ${APP_SERVICES} )

  include_directories(
    "/usr/local/Cellar/glew/1.10.0/include"
    "/usr/local/include"
  )

  link_directories(
    "/usr/local/Cellar/glew/1.10.0/lib"
    "/usr/local/lib"
  )

else()
  if (MSVC)
    # glob all the root level .cpp files
    file(GLOB ROOT_SRC "*.cpp" "protocol/*.cpp")

    include_directories(
      "/projects/glew-1.10.0/include"
      "/projects/glfw/include"
    )

    link_directories(
      "/projects/glfw/src/debug"
    )

    # add precompiled header, and force include it on all the root level .cpp files
    foreach( src_file ${ROOT_SRC} )
        set_source_files_properties(${src_file} PROPERTIES COMPILE_FLAGS "/Yuprecompiled.hpp /FIprecompiled.hpp")
    endforeach( src_file ${ROOT_SRC} )

    set_source_files_properties(precompiled.cpp PROPERTIES COMPILE_FLAGS "/Ycprecompiled.hpp")

    # Force static runtime libraries
    foreach(flag CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_DEBUG)
      STRING(REPLACE "/MD"  "/MT" "${flag}" "${${flag}}")
      SET("${flag}" "${${flag}}")
    endforeach()
   
    target_link_libraries(${PROJECT_NAME}
      debug ${BRISTOL_MAIN_LIBRARY_DEBUG} optimized ${BRISTOL_MAIN_LIBRARY_RELEASE}
      debug ${PROTOBUF_LIBRARY_DEBUG} optimized ${PROTOBUF_LIBRARY}
      )
  endif(MSVC)
endif()
