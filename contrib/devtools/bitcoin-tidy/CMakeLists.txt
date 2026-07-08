cmake_minimum_required(VERSION 3.22)

project(bitcoin-tidy
    VERSION
    1.0.0
    DESCRIPTION "clang-tidy checks for Bitcoin Core"
    LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

set(CMAKE_DISABLE_FIND_PACKAGE_CURL ON)
set(CMAKE_DISABLE_FIND_PACKAGE_FFI ON)
set(CMAKE_DISABLE_FIND_PACKAGE_LibEdit ON)
set(CMAKE_DISABLE_FIND_PACKAGE_LibXml2 ON)
set(CMAKE_DISABLE_FIND_PACKAGE_Terminfo ON)
set(CMAKE_DISABLE_FIND_PACKAGE_ZLIB ON)
set(CMAKE_DISABLE_FIND_PACKAGE_zstd ON)

find_package(LLVM REQUIRED CONFIG)
find_program(CLANG_TIDY_EXE REQUIRED NAMES "clang-tidy-${LLVM_VERSION_MAJOR}" "clang-tidy" HINTS ${LLVM_TOOLS_BINARY_DIR})
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXE}")

add_library(bitcoin-tidy MODULE bitcoin-tidy.cpp nontrivial-threadlocal.cpp field-observers.cpp)
target_include_directories(bitcoin-tidy SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})

# Disable RTTI and exceptions as necessary
if (MSVC)
    target_compile_options(bitcoin-tidy PRIVATE /GR-)
else()
    target_compile_options(bitcoin-tidy PRIVATE -fno-rtti)
    target_compile_options(bitcoin-tidy PRIVATE -fno-exceptions)
endif()

if(CMAKE_HOST_APPLE)
    # ld64 expects no undefined symbols by default
    target_link_options(bitcoin-tidy PRIVATE -Wl,-flat_namespace)
    target_link_options(bitcoin-tidy PRIVATE -Wl,-undefined -Wl,suppress)
endif()

# Add warnings
if (MSVC)
    target_compile_options(bitcoin-tidy PRIVATE /W4)
else()
    target_compile_options(bitcoin-tidy PRIVATE -Wall)
    target_compile_options(bitcoin-tidy PRIVATE -Wextra)
endif()

if(CMAKE_VERSION VERSION_LESS 3.27)
    set(CLANG_TIDY_COMMAND "${CLANG_TIDY_EXE}" "--load=${CMAKE_BINARY_DIR}/${CMAKE_SHARED_MODULE_PREFIX}bitcoin-tidy${CMAKE_SHARED_MODULE_SUFFIX}" "-checks=-*,bitcoin-nontrivial-threadlocal")
else()
    # CLANG_TIDY_COMMAND supports generator expressions as of 3.27
    set(CLANG_TIDY_COMMAND "${CLANG_TIDY_EXE}" "--load=$<TARGET_FILE:bitcoin-tidy>" "-checks=-*,bitcoin-nontrivial-threadlocal")
endif()

# Create a dummy library that runs clang-tidy tests as a side-effect of building
add_library(bitcoin-tidy-tests OBJECT EXCLUDE_FROM_ALL example_nontrivial-threadlocal.cpp)
add_dependencies(bitcoin-tidy-tests bitcoin-tidy)

set_target_properties(bitcoin-tidy-tests PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")

# Apply the field-observers fix-its to the example, require golden output, compile the result.
add_custom_command(
    OUTPUT field-observers-test.cpp
    COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/example_field-observers.cpp" field-observers-test.cpp
    COMMAND "${CLANG_TIDY_EXE}" "--load=$<TARGET_FILE:bitcoin-tidy>" "-checks=-*,bitcoin-field-observers" "-config={CheckOptions: {bitcoin-field-observers.QualifiedFieldName: '::Tx::vout', bitcoin-field-observers.AccessorDeclaration: 'int vout() const { return m_vout; }'}}" "-fix" field-observers-test.cpp "--" "-std=c++20"
    COMMAND "${CMAKE_COMMAND}" -E compare_files --ignore-eol field-observers-test.cpp "${CMAKE_CURRENT_SOURCE_DIR}/expected_field-observers.cpp"
    DEPENDS bitcoin-tidy "${CMAKE_CURRENT_SOURCE_DIR}/example_field-observers.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/expected_field-observers.cpp"
    VERBATIM
)
target_sources(bitcoin-tidy-tests PRIVATE field-observers-test.cpp)

install(TARGETS bitcoin-tidy LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
