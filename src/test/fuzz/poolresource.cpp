// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <random.h>
#include <span.h>
#include <support/allocators/pool.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/poolresourcetester.h>
#include <util/byte_units.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <span>
#include <tuple>
#include <vector>

namespace {

template <std::size_t MAX_BLOCK_SIZE_BYTES, std::size_t ALIGN_BYTES>
class PoolResourceFuzzer
{
    FuzzedDataProvider& m_provider;
    PoolResource<MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES> m_test_resource;
    uint64_t m_sequence{0};
    size_t m_total_allocated{};

    struct Entry {
        std::span<std::byte> span;
        size_t alignment;
        uint64_t seed;

        Entry(std::span<std::byte> s, size_t a, uint64_t se) : span(s), alignment(a), seed(se) {}
    };

    std::vector<Entry> m_entries;

    void AssertLiveAllocationsDisjoint() const
    {
        std::vector<std::span<std::byte>> live_spans;
        live_spans.reserve(m_entries.size());
        for (const auto& entry : m_entries) {
            live_spans.push_back(entry.span);
        }
        PoolResourceTester::CheckLiveSpansDisjoint(live_spans);
    }

    void AssertAllocationTransition(size_t size,
                                    size_t alignment,
                                    const std::vector<size_t>& free_lists_before,
                                    size_t available_before,
                                    size_t chunks_before) const
    {
        const auto free_lists_after{PoolResourceTester::FreeListSizes(m_test_resource)};
        const size_t available_after{PoolResourceTester::AvailableMemoryFromChunk(m_test_resource)};
        const size_t chunks_after{m_test_resource.NumAllocatedChunks()};
        const bool uses_pool{PoolResourceTester::IsFreeListUsable<MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>(size, alignment)};

        if (!uses_pool) {
            assert(free_lists_after == free_lists_before);
            assert(available_after == available_before);
            assert(chunks_after == chunks_before);
            return;
        }

        const size_t free_list_index{PoolResourceTester::FreeListIndex<MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>(size)};
        const size_t rounded_size{PoolResourceTester::RoundedBytes<MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>(size)};
        const size_t elem_align{PoolResourceTester::ElemAlignBytes<MAX_BLOCK_SIZE_BYTES, ALIGN_BYTES>()};
        assert(free_list_index > 0);
        assert(free_list_index < free_lists_before.size());
        assert(rounded_size >= std::max(size, elem_align));

        if (free_lists_before[free_list_index] > 0) {
            auto expected{free_lists_before};
            --expected[free_list_index];
            assert(free_lists_after == expected);
            assert(available_after == available_before);
            assert(chunks_after == chunks_before);
        } else if (rounded_size <= available_before) {
            assert(free_lists_after == free_lists_before);
            assert(available_after + rounded_size == available_before);
            assert(chunks_after == chunks_before);
        } else {
            auto expected{free_lists_before};
            assert((available_before % elem_align) == 0);
            if (available_before > 0) {
                const size_t leftover_index{available_before / elem_align};
                assert(leftover_index > 0);
                assert(leftover_index < expected.size());
                ++expected[leftover_index];
            }
            assert(free_lists_after == expected);
            assert(available_after + rounded_size == m_test_resource.ChunkSizeBytes());
            assert(chunks_after == chunks_before + 1);
        }
    }

public:
    PoolResourceFuzzer(FuzzedDataProvider& provider)
        : m_provider{provider},
          m_test_resource{provider.ConsumeIntegralInRange<size_t>(MAX_BLOCK_SIZE_BYTES, 262144)}
    {
        PoolResourceTester::CheckResourceInvariants(m_test_resource);
    }

    void Allocate(size_t size, size_t alignment)
    {
        assert(alignment > 0);                      // Alignment must be at least 1.
        assert((alignment & (alignment - 1)) == 0); // Alignment must be power of 2.

        const auto free_lists_before{PoolResourceTester::FreeListSizes(m_test_resource)};
        const size_t available_before{PoolResourceTester::AvailableMemoryFromChunk(m_test_resource)};
        const size_t chunks_before{m_test_resource.NumAllocatedChunks()};
        auto span = std::span(static_cast<std::byte*>(m_test_resource.Allocate(size, alignment)), size);
        PoolResourceTester::CheckResourceInvariants(m_test_resource);
        AssertAllocationTransition(size, alignment, free_lists_before, available_before, chunks_before);
        m_total_allocated += size;

        auto ptr_val = reinterpret_cast<std::uintptr_t>(span.data());
        assert((ptr_val & (alignment - 1)) == 0);

        uint64_t seed = m_sequence++;
        RandomContentFill(m_entries.emplace_back(span, alignment, seed));
        AssertLiveAllocationsDisjoint();
    }

    void
    Allocate()
    {
        if (m_total_allocated > 16_MiB) return;
        size_t alignment_bits = m_provider.ConsumeIntegralInRange<size_t>(0, 7);
        size_t alignment = size_t{1} << alignment_bits;
        size_t size{0};
        if (!m_provider.ConsumeBool()) {
            size_t size_bits = m_provider.ConsumeIntegralInRange<size_t>(0, 16);
            size = m_provider.ConsumeIntegralInRange<size_t>(size_t{1} << size_bits,
                                                             (size_t{1} << (size_bits + 1)) - 1U);
        }
        Allocate(size, alignment);
    }

    void RandomContentFill(Entry& entry)
    {
        InsecureRandomContext(entry.seed).fillrand(entry.span);
    }

    void RandomContentCheck(const Entry& entry)
    {
        std::vector<std::byte> expect(entry.span.size());
        InsecureRandomContext(entry.seed).fillrand(expect);
        assert(std::ranges::equal(entry.span, expect));
    }

    void Deallocate(const Entry& entry)
    {
        auto ptr_val = reinterpret_cast<std::uintptr_t>(entry.span.data());
        assert((ptr_val & (entry.alignment - 1)) == 0);
        RandomContentCheck(entry);
        m_total_allocated -= entry.span.size();
        m_test_resource.Deallocate(entry.span.data(), entry.span.size(), entry.alignment);
        PoolResourceTester::CheckResourceInvariants(m_test_resource);
    }

    void Deallocate()
    {
        if (m_entries.empty()) {
            return;
        }

        size_t idx = m_provider.ConsumeIntegralInRange<size_t>(0, m_entries.size() - 1);
        Deallocate(m_entries[idx]);
        if (idx != m_entries.size() - 1) {
            m_entries[idx] = std::move(m_entries.back());
        }
        m_entries.pop_back();
        AssertLiveAllocationsDisjoint();
    }

    void Clear()
    {
        while (!m_entries.empty()) {
            Deallocate();
        }

        PoolResourceTester::CheckAllDataAccountedFor(m_test_resource);
    }

    void Fuzz()
    {
        LIMITED_WHILE (m_provider.ConsumeBool(), 10000) {
            CallOneOf(
                m_provider,
                [&] { Allocate(); },
                [&] { Deallocate(); });
        }
        Clear();
    }
};


} // namespace

FUZZ_TARGET(pool_resource)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    CallOneOf(
        provider,
        [&] { PoolResourceFuzzer<128, 1>{provider}.Fuzz(); },
        [&] { PoolResourceFuzzer<128, 2>{provider}.Fuzz(); },
        [&] { PoolResourceFuzzer<128, 4>{provider}.Fuzz(); },
        [&] { PoolResourceFuzzer<128, 8>{provider}.Fuzz(); },

        [&] { PoolResourceFuzzer<8, 8>{provider}.Fuzz(); },
        [&] { PoolResourceFuzzer<16, 16>{provider}.Fuzz(); },

        [&] { PoolResourceFuzzer<256, alignof(max_align_t)>{provider}.Fuzz(); },
        [&] { PoolResourceFuzzer<256, 64>{provider}.Fuzz(); });
}
