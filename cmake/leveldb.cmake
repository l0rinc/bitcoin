# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

set(ROCKSDB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(WITH_BENCHMARK OFF CACHE BOOL "" FORCE)
set(WITH_BENCHMARK_TOOLS OFF CACHE BOOL "" FORCE)
set(WITH_CORE_TOOLS OFF CACHE BOOL "" FORCE)
set(WITH_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WITH_GFLAGS OFF CACHE BOOL "" FORCE)
set(WITH_JNI OFF CACHE BOOL "" FORCE)
set(WITH_LIBURING OFF CACHE BOOL "" FORCE)
set(WITH_TESTS OFF CACHE BOOL "" FORCE)
set(WITH_TOOLS OFF CACHE BOOL "" FORCE)
set(WITH_TRACE_TOOLS OFF CACHE BOOL "" FORCE)
set(FAIL_ON_WARNINGS OFF CACHE BOOL "" FORCE)
set(USE_RTTI ON CACHE STRING "" FORCE)

add_subdirectory(
  ${PROJECT_SOURCE_DIR}/src/leveldb
  ${PROJECT_BINARY_DIR}/src/leveldb
  EXCLUDE_FROM_ALL
)

add_library(leveldb INTERFACE)
target_link_libraries(leveldb INTERFACE rocksdb)
target_compile_definitions(rocksdb PUBLIC ROCKSDB_NAMESPACE=42)
target_include_directories(rocksdb BEFORE PRIVATE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/leveldb>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/leveldb/include>
)

set_target_properties(rocksdb PROPERTIES
  EXPORT_COMPILE_COMMANDS OFF
)
