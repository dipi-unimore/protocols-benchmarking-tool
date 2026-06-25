cmake_minimum_required(VERSION 3.28)
project(pb_tool VERSION 0.1.0 LANGUAGES CXX C)

# Allow fetched dependencies that declare old cmake_minimum_required (e.g. yaml-cpp 0.8.0 uses 2.8.12)
set(CMAKE_POLICY_VERSION_MINIMUM "3.5")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

include(cmake/CompilerOptions.cmake)
include(cmake/FetchDependencies.cmake)

# protobuf_generate is not auto-included when building protobuf via FetchContent (no install step)
include(${protobuf_SOURCE_DIR}/cmake/protobuf-generate.cmake)

# Protobuf code generation
add_library(pbt_proto STATIC)
target_include_directories(pbt_proto PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/proto
    ${protobuf_SOURCE_DIR}/src)
target_link_libraries(pbt_proto PUBLIC
    protobuf::libprotobuf
    pbt_compiler_options)
protobuf_generate(
    TARGET    pbt_proto
    PROTOS    ${CMAKE_CURRENT_SOURCE_DIR}/proto/pbt_message.proto
    LANGUAGE  cpp)

add_subdirectory(src)
add_subdirectory(app)

option(PBT_BUILD_TESTS "Build tests" ON)
if(PBT_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
