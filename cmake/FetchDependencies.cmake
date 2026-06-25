include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# nlohmann_json
FetchContent_Declare(nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
  GIT_SHALLOW    TRUE)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install    OFF CACHE BOOL "" FORCE)

# yaml-cpp
FetchContent_Declare(yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG        0.8.0
  GIT_SHALLOW    TRUE)
set(YAML_CPP_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS   OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL       OFF CACHE BOOL "" FORCE)

# spdlog
FetchContent_Declare(spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.14.1
  GIT_SHALLOW    TRUE)
set(SPDLOG_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL        OFF CACHE BOOL "" FORCE)
# Use C++23 std::format backend to avoid fmt consteval issues with Apple Clang 21
set(SPDLOG_USE_STD_FORMAT ON  CACHE BOOL "" FORCE)

# googletest
FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
  GIT_SHALLOW    TRUE)
set(BUILD_GMOCK              ON  CACHE BOOL "" FORCE)
set(INSTALL_GTEST            OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt   OFF CACHE BOOL "" FORCE)

# protobuf
FetchContent_Declare(protobuf
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
  GIT_TAG        v27.3
  GIT_SHALLOW    TRUE)
set(protobuf_BUILD_TESTS          OFF CACHE BOOL "" FORCE)
set(protobuf_INSTALL              OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_PROTOC_BINARIES ON  CACHE BOOL "" FORCE)
set(protobuf_BUILD_SHARED_LIBS    OFF CACHE BOOL "" FORCE)
set(ABSL_ENABLE_INSTALL           OFF CACHE BOOL "" FORCE)
set(utf8_range_ENABLE_TESTS       OFF CACHE BOOL "" FORCE)
set(utf8_range_ENABLE_INSTALL     OFF CACHE BOOL "" FORCE)

# zstd
FetchContent_Declare(zstd
  GIT_REPOSITORY https://github.com/facebook/zstd.git
  GIT_TAG        v1.5.6
  GIT_SHALLOW    TRUE
  SOURCE_SUBDIR  build/cmake)
set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_CONTRIB  OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_SHARED   OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_STATIC   ON  CACHE BOOL "" FORCE)

# libzmq
FetchContent_Declare(libzmq
  GIT_REPOSITORY https://github.com/zeromq/libzmq.git
  GIT_TAG        v4.3.5
  GIT_SHALLOW    TRUE)
set(ZMQ_BUILD_TESTS        OFF CACHE BOOL "" FORCE)
set(ENABLE_CPACK           OFF CACHE BOOL "" FORCE)
set(WITH_DOCS              OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED           OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC           ON  CACHE BOOL "" FORCE)
set(LIBZMQ_WERROR          OFF CACHE BOOL "" FORCE)
set(WITH_PERF_TOOL         OFF CACHE BOOL "" FORCE)

# cppzmq (header-only)
FetchContent_Declare(cppzmq
  GIT_REPOSITORY https://github.com/zeromq/cppzmq.git
  GIT_TAG        v4.10.0
  GIT_SHALLOW    TRUE)
set(CPPZMQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)

# paho-mqtt-cpp with bundled paho-mqtt-c (avoids find_package(eclipse-paho-mqtt-c))
FetchContent_Declare(paho-mqtt-cpp
  GIT_REPOSITORY https://github.com/eclipse/paho.mqtt.cpp.git
  GIT_TAG        v1.4.0
  GIT_SHALLOW    TRUE)
set(PAHO_BUILD_SHARED        OFF CACHE BOOL "" FORCE)
set(PAHO_BUILD_STATIC        ON  CACHE BOOL "" FORCE)
set(PAHO_BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(PAHO_BUILD_SAMPLES       OFF CACHE BOOL "" FORCE)
set(PAHO_BUILD_EXAMPLES      OFF CACHE BOOL "" FORCE)
set(PAHO_BUILD_TESTS         OFF CACHE BOOL "" FORCE)
set(PAHO_WITH_SSL            OFF CACHE BOOL "" FORCE)
# Use paho-mqtt-cpp's own bundled paho-mqtt-c submodule instead of find_package
set(PAHO_WITH_MQTT_C         ON  CACHE BOOL "" FORCE)
# Bundled paho-mqtt-c options (shared with paho-mqtt-c namespace)
set(PAHO_ENABLE_TESTING      OFF CACHE BOOL "" FORCE)
set(PAHO_HIGH_PERFORMANCE    ON  CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(
  nlohmann_json
  yaml-cpp
  spdlog
  googletest
  protobuf
  zstd
  libzmq
  cppzmq
  paho-mqtt-cpp)

# tinycbor — no CMakeLists.txt upstream; inject our wrapper
FetchContent_Declare(tinycbor
  GIT_REPOSITORY https://github.com/intel/tinycbor.git
  GIT_TAG        v0.6.0
  GIT_SHALLOW    TRUE)
FetchContent_GetProperties(tinycbor)
if(NOT tinycbor_POPULATED)
  FetchContent_Populate(tinycbor)
  configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/patches/tinycbor_CMakeLists.txt
    ${tinycbor_SOURCE_DIR}/CMakeLists.txt
    COPYONLY)
  add_subdirectory(${tinycbor_SOURCE_DIR} ${tinycbor_BINARY_DIR})
endif()
