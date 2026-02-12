cmake_minimum_required(VERSION 3.18)
project(milvus_dbpu_plugin LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(DBPU_PLUGIN_BUILD_TESTS "Build plugin tests" OFF)

add_library(dbpu_plugin SHARED
  src/knowhere_hooks.cpp
  src/runtime_loader.cpp
  src/interceptor.cpp
  src/logger.cpp
  src/profiler.cpp
  src/offload.cpp
)

target_include_directories(dbpu_plugin PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# We use dlopen/dlsym
if(APPLE)
  target_link_libraries(dbpu_plugin PRIVATE "-Wl,-undefined,dynamic_lookup")
else()
  target_link_libraries(dbpu_plugin PRIVATE dl)
endif()

target_compile_definitions(dbpu_plugin PUBLIC DBPU_PLUGIN=1)

# (선택) 테스트
if(DBPU_PLUGIN_BUILD_TESTS)
  add_executable(test_plugin tests/test_plugin.cpp)
  target_link_libraries(test_plugin PRIVATE dbpu_plugin)
endif()


add_executable(test_loader tests/test_loader.cpp)
target_link_libraries(test_loader PRIVATE dbpu_plugin)