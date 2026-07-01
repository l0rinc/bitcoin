// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <random.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

FUZZ_TARGET(random)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const uint256 seed{ConsumeUInt256(fuzzed_data_provider)};
    FastRandomContext fast_random_context{seed};
    FastRandomContext mirror_context{seed};

    assert(fast_random_context.rand64() == mirror_context.rand64());

    const int bits{fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 64)};
    const uint64_t rand_bits{fast_random_context.randbits(bits)};
    assert(rand_bits == mirror_context.randbits(bits));
    if (bits < 64) assert(rand_bits >> bits == 0);

    const uint64_t range{fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(FastRandomContext::min() + 1, FastRandomContext::max())};
    const uint64_t rand_range{fast_random_context.randrange(range)};
    assert(rand_range == mirror_context.randrange(range));
    assert(rand_range < range);

    const size_t randbytes_len{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 1024)};
    const std::vector<uint8_t> rand_bytes{fast_random_context.randbytes<uint8_t>(randbytes_len)};
    assert(rand_bytes == mirror_context.randbytes<uint8_t>(randbytes_len));
    assert(rand_bytes.size() == randbytes_len);

    assert(fast_random_context.rand32() == mirror_context.rand32());
    assert(fast_random_context.rand256() == mirror_context.rand256());
    assert(fast_random_context.randbool() == mirror_context.randbool());
    assert(fast_random_context() == mirror_context());

    std::vector<int64_t> integrals = ConsumeRandomLengthIntegralVector<int64_t>(fuzzed_data_provider);
    std::vector<int64_t> mirror_integrals{integrals};
    std::vector<int64_t> sorted_integrals{integrals};
    std::shuffle(integrals.begin(), integrals.end(), fast_random_context);
    std::shuffle(mirror_integrals.begin(), mirror_integrals.end(), mirror_context);
    assert(integrals == mirror_integrals);

    std::vector<int64_t> sorted_shuffled_integrals{integrals};
    std::sort(sorted_integrals.begin(), sorted_integrals.end());
    std::sort(sorted_shuffled_integrals.begin(), sorted_shuffled_integrals.end());
    assert(sorted_integrals == sorted_shuffled_integrals);
}
