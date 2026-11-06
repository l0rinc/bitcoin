# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

include_guard(GLOBAL)

# Checks for C++ features required to compile Bitcoin Core.

include(CheckCXXSourceCompiles)

function(check_cxx_64bit config)
  set(CMAKE_TRY_COMPILE_CONFIGURATION "${config}")
  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
  list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES CMAKE_CONFIGURATION_TYPES)
  # Recheck after flag changes, including APPEND_CXXFLAGS and APPEND_CPPFLAGS
  unset(HAVE_64BIT_TARGET CACHE)
  check_cxx_source_compiles("
    static_assert(sizeof(void*) == 8);
    int main() {}
  " HAVE_64BIT_TARGET)
  if(NOT HAVE_64BIT_TARGET)
    message(FATAL_ERROR "Bitcoin Core requires a 64-bit target.")
  endif()
endfunction()

function(check_cxx_features)
  set(CMAKE_REQUIRED_QUIET TRUE)

  message(STATUS "Checking for required C++ features")

  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Generic")
    get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
      foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
        check_cxx_64bit("${config}")
      endforeach()
    else()
      check_cxx_64bit("${CMAKE_BUILD_TYPE}")
    endif()
  endif()

  # Checks for Class Template Argument Deduction for alias templates.
  check_cxx_source_compiles("
    template<class T> struct Template { Template(T) {} };
    template<class T> using Alias = Template<T>;

    int main() {
      Alias value{42};
      return sizeof(value);
    }
  " HAVE_CTAD_FOR_ALIAS_TEMPLATES)

  if(NOT HAVE_CTAD_FOR_ALIAS_TEMPLATES)
    message(FATAL_ERROR
      "Compiler lacks Class Template Argument Deduction (CTAD) for alias templates.\n"
      "You are probably using an old compiler version\n"
      "The recommended compiler versions can be checked in\n"
      "doc/dependencies.md#compiler.\n"
    )
  endif()

  message(STATUS "Checking for required C++ features - done")

endfunction()
