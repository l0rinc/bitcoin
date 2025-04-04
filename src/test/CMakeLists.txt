# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

# Do not use generator expressions in test sources because the
# SOURCES property is processed to gather test suite macros.
add_executable(test_bitcoin
  main.cpp
  streams_tests.cpp
)

include(TargetDataSources)
target_json_data_sources(test_bitcoin
  data/base58_encode_decode.json
  data/bip341_wallet_vectors.json
  data/blockfilters.json
  data/key_io_invalid.json
  data/key_io_valid.json
  data/script_tests.json
  data/sighash.json
  data/tx_invalid.json
  data/tx_valid.json
)
target_raw_data_sources(test_bitcoin NAMESPACE test::data
  data/asmap.raw
)

target_link_libraries(test_bitcoin
  core_interface
  test_util
  bitcoin_cli
  bitcoin_node
  bitcoin_consensus
  minisketch
  secp256k1
  Boost::headers
  libevent::extra
)

if(ENABLE_WALLET)
  add_subdirectory(${PROJECT_SOURCE_DIR}/src/wallet/test wallet)
endif()

if(WITH_MULTIPROCESS)
  target_link_libraries(bitcoin_ipc_test
    PRIVATE
      core_interface
      univalue
  )

  target_sources(test_bitcoin
    PRIVATE
      ipc_tests.cpp
  )
  target_link_libraries(test_bitcoin bitcoin_ipc_test bitcoin_ipc)
endif()

function(add_boost_test source_file)
  if(NOT EXISTS ${source_file})
    return()
  endif()

  file(READ "${source_file}" source_file_content)
  string(REGEX
    MATCH "(BOOST_FIXTURE_TEST_SUITE|BOOST_AUTO_TEST_SUITE)\\(([A-Za-z0-9_]+)"
    test_suite_macro "${source_file_content}"
  )
  string(REGEX
    REPLACE "(BOOST_FIXTURE_TEST_SUITE|BOOST_AUTO_TEST_SUITE)\\(" ""
    test_suite_name "${test_suite_macro}"
  )
  if(test_suite_name)
    add_test(NAME ${test_suite_name}
      COMMAND test_bitcoin --run_test=${test_suite_name} --catch_system_error=no --log_level=test_suite -- DEBUG_LOG_OUT
    )
    set_property(TEST ${test_suite_name} PROPERTY
      SKIP_REGULAR_EXPRESSION "no test cases matching filter"
    )
  endif()
endfunction()

function(add_all_test_targets)
  get_target_property(test_source_dir test_bitcoin SOURCE_DIR)
  get_target_property(test_sources test_bitcoin SOURCES)
  foreach(test_source ${test_sources})
    cmake_path(IS_RELATIVE test_source result)
    if(result)
      cmake_path(APPEND test_source_dir ${test_source} OUTPUT_VARIABLE test_source)
    endif()
    add_boost_test(${test_source})
  endforeach()
endfunction()

add_all_test_targets()

install_binary_component(test_bitcoin)
