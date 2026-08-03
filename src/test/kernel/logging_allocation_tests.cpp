// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying file COPYING or https://opensource.org/license/mit/.

#include <kernel/bitcoinkernel.h>

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <new>

namespace {
std::atomic_bool g_fail_next_allocation{false};
std::atomic_int g_destroy_calls{0};

void* Allocate(const std::size_t size)
{
    if (g_fail_next_allocation.exchange(false, std::memory_order_relaxed)) {
        throw std::bad_alloc{};
    }
    if (void* ptr{std::malloc(size)}) return ptr;
    throw std::bad_alloc{};
}

void LogCallback(void*, const char*, size_t) {}

void DestroyCallback(void* user_data)
{
    ++g_destroy_calls;
    delete static_cast<int*>(user_data);
}
} // namespace

void* operator new(const std::size_t size)
{
    return Allocate(size);
}

void* operator new[](const std::size_t size)
{
    return Allocate(size);
}

void operator delete(void* ptr) noexcept
{
    std::free(ptr);
}

void operator delete[](void* ptr) noexcept
{
    std::free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept
{
    std::free(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept
{
    std::free(ptr);
}

int main()
{
    int* const user_data{new int{0}};
    g_fail_next_allocation.store(true, std::memory_order_relaxed);

    const auto connection{btck_logging_connection_create(LogCallback, user_data, DestroyCallback)};
    if (connection != nullptr) {
        btck_logging_connection_destroy(connection);
        return 1;
    }
    return g_destroy_calls == 1 ? 0 : 1;
}
