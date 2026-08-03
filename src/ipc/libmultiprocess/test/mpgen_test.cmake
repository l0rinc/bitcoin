# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

file(MAKE_DIRECTORY "${OUTPUT_DIR}")
execute_process(
  COMMAND "${MPGEN}" "${SRC_PREFIX}" "${INCLUDE_PREFIX}" "${SOURCE_FILE}" "${IMPORT_PATH}"
  WORKING_DIRECTORY "${OUTPUT_DIR}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE error)

if(result EQUAL 0)
  message(FATAL_ERROR "mpgen accepted an unsafe include annotation")
endif()
if(NOT error MATCHES "include annotation contains an invalid path character")
  message(FATAL_ERROR "mpgen failed for an unexpected reason: ${error}${output}")
endif()

get_filename_component(schema_name "${SOURCE_FILE}" NAME)
foreach(suffix IN ITEMS .proxy-client.c++ .proxy-server.c++ .proxy-types.c++ .proxy-types.h .proxy.h)
  if(EXISTS "${OUTPUT_DIR}/${schema_name}${suffix}")
    message(FATAL_ERROR "mpgen wrote ${schema_name}${suffix} after rejecting the schema")
  endif()
endforeach()
