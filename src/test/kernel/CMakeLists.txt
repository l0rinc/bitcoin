add_executable(test_kernel
    test_kernel.cpp
)

target_link_libraries(test_kernel
  PRIVATE
    core_interface
    bitcoinkernel
    Boost::headers
)

add_windows_application_manifest(test_kernel)

add_test(NAME test_kernel COMMAND test_kernel)

add_executable(test_kernel_logging_allocation
    logging_allocation_tests.cpp
)

target_link_libraries(test_kernel_logging_allocation
  PRIVATE
    core_interface
    bitcoinkernel
)

add_windows_application_manifest(test_kernel_logging_allocation)

add_test(NAME test_kernel_logging_allocation COMMAND test_kernel_logging_allocation)
