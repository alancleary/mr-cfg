cmake_minimum_required(VERSION 3.15 FATAL_ERROR)


# set the build type to release if not specified
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()


# setup the CMake project
project("MR-CFG"
        VERSION 0.0.1
        DESCRIPTION "Computes a context-free grammar from a string's maximal repeats"
        LANGUAGES CXX
)


# set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# install external libraries
include(FetchContent)

# Roaring
FetchContent_Declare(
  roaring
  GIT_REPOSITORY https://github.com/RoaringBitmap/CRoaring.git
  GIT_TAG v0.8.0
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE)

set(ENABLE_ROARING_TESTS OFF CACHE INTERNAL "")
set(ROARING_BUILD_STATIC ON CACHE INTERNAL "")

FetchContent_MakeAvailable(roaring)

#FetchContent_GetProperties(roaring)
SET(CPP_ROARING_HEADERS ${roaring_SOURCE_DIR}/cpp/roaring64map.hh  ${roaring_SOURCE_DIR}/cpp/roaring.hh)
file(COPY ${CPP_ROARING_HEADERS} DESTINATION ${roaring_SOURCE_DIR}/include/roaring)

# SDSL
FetchContent_Declare(
  sdsl
  GIT_REPOSITORY https://github.com/xxsds/sdsl-lite.git
  GIT_TAG v3.0.1
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
  CONFIGURE_COMMAND ""
  BUILD_COMMAND "")

#FetchContent_MakeAvailable(sdsl)

FetchContent_GetProperties(sdsl)
if(NOT sdsl_POPULATED)
  FetchContent_Populate(sdsl)
endif()


# locate all source files
file(GLOB SOURCES src/*.cpp)

# compile the sources into an executable
add_executable(${PROJECT_NAME} ${SOURCES})

# specify include directories
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
# TODO: this should be done with target_include_directories
#include_directories("${sdsl_SOURCE_DIR}/include")


# link the libraries
target_link_libraries(${PROJECT_NAME} PUBLIC roaring)
target_include_directories(${PROJECT_NAME} PRIVATE ${sdsl_SOURCE_DIR}/include)
