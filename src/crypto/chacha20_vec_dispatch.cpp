// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20_vec.h>

#ifdef ENABLE_CHACHA20_VEC
#include <crypto/chacha20_vec_128impl.h>

#include <cassert>

namespace chacha20_vec {

namespace {

[[maybe_unused]] ALWAYS_INLINE void CryptX86_64(std::span<const std::byte>& in_bytes, std::span<std::byte>& out_bytes, std::span<const uint32_t, STATE_WORDS> input) noexcept
{
    assert(in_bytes.size() == out_bytes.size());
    chacha20_vec128::ChaCha20Vectorized crypter{input};

    while (in_bytes.size() >= BLOCKLEN * MIN_BLOCKS) {
        const size_t blocks{out_bytes.size() / BLOCKLEN};
        switch (blocks) {
        case 2: crypter.CryptStates<2>(in_bytes, out_bytes); break;
        default: crypter.CryptStates<3>(in_bytes, out_bytes); break;
        case 4:
        case 8: crypter.CryptStates<4>(in_bytes, out_bytes); break; // 4 is faster than 3 + 1; 4 + 4 is faster than 3 + 3 + 2
        }
    }
}

[[maybe_unused]] ALWAYS_INLINE void CryptAArch64(std::span<const std::byte>& in_bytes, std::span<std::byte>& out_bytes, std::span<const uint32_t, STATE_WORDS> input) noexcept
{
    assert(in_bytes.size() == out_bytes.size());
    chacha20_vec128::ChaCha20Vectorized crypter{input};

    // Exact groups through 13 are fastest; 14 and 15 are faster as 8+6 and 8+7
    while (in_bytes.size() >= BLOCKLEN) {
        const size_t blocks{out_bytes.size() / BLOCKLEN};
        if (blocks < MIN_BLOCKS) break;
        switch (blocks) {
        case 3: crypter.CryptStates<3>(in_bytes, out_bytes); break;
        case 4: crypter.CryptStates<4>(in_bytes, out_bytes); break;
        case 5: crypter.CryptStates<5>(in_bytes, out_bytes); break;
        case 6: crypter.CryptStates<6>(in_bytes, out_bytes); break;
        case 7: crypter.CryptStates<7>(in_bytes, out_bytes); break;
        default: crypter.CryptStates<8>(in_bytes, out_bytes); break;
        case 9: crypter.CryptStates<9>(in_bytes, out_bytes); break;
        case 10: crypter.CryptStates<10>(in_bytes, out_bytes); break;
        case 11: crypter.CryptStates<11>(in_bytes, out_bytes); break;
        case 12: crypter.CryptStates<12>(in_bytes, out_bytes); break;
        case 13: crypter.CryptStates<13>(in_bytes, out_bytes); break;
        }
    }
}

} // namespace

void chacha20_crypt_vectorized(std::span<const std::byte>& in_bytes, std::span<std::byte>& out_bytes, std::span<const uint32_t, STATE_WORDS> input) noexcept
{
    if constexpr (TARGET == VectorTarget::X86_64) {
        CryptX86_64(in_bytes, out_bytes, input);
    } else if constexpr (TARGET == VectorTarget::AARCH64) {
        CryptAArch64(in_bytes, out_bytes, input);
    }
}

} // namespace chacha20_vec

#endif // ENABLE_CHACHA20_VEC
