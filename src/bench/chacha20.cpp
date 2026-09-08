// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <crypto/chacha20.h>
#include <crypto/chacha20poly1305.h>
// IWYU incorrectly suggests removing this header.
// See https://github.com/include-what-you-use/include-what-you-use/issues/2014.
#include <util/byte_units.h> // IWYU pragma: keep

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

/* Number of bytes to process per iteration */
static const uint64_t BUFFER_SIZE_TINY  = 64;
static const uint64_t BUFFER_SIZE_SMALL = 256;
static const uint64_t BUFFER_SIZE_LARGE{1_MiB};

static void CHACHA20(benchmark::Bench& bench, size_t buffersize)
{
    std::vector<std::byte> key(32, {});
    ChaCha20 ctx(key);
    ctx.Seek({0, 0}, 0);
    std::vector<std::byte> in(buffersize, {});
    std::vector<std::byte> out(buffersize, {});
    bench.batch(in.size()).unit("byte").run([&] {
        ctx.Crypt(in, out);
    });
}

static void FSCHACHA20POLY1305(benchmark::Bench& bench, size_t buffersize)
{
    std::vector<std::byte> key(32);
    FSChaCha20Poly1305 ctx(key, 224);
    std::vector<std::byte> in(buffersize);
    std::vector<std::byte> aad;
    std::vector<std::byte> out(buffersize + FSChaCha20Poly1305::EXPANSION);
    bench.batch(in.size()).unit("byte").run([&] {
        ctx.Encrypt(in, aad, out);
    });
}

static void CHACHA20_64BYTES(benchmark::Bench& bench)
{
    CHACHA20(bench, BUFFER_SIZE_TINY);
}

static void CHACHA20_128BYTES(benchmark::Bench& bench)
{
    CHACHA20(bench, ChaCha20Aligned::BLOCKLEN * 2);
}

static void CHACHA20_192BYTES(benchmark::Bench& bench)
{
    CHACHA20(bench, ChaCha20Aligned::BLOCKLEN * 3);
}

static void CHACHA20_256BYTES(benchmark::Bench& bench)
{
    CHACHA20(bench, BUFFER_SIZE_SMALL);
}

// 12-15 blocks probe the AArch64 exact-group boundary (see the chacha20_vec dispatch)
static void CHACHA20_768BYTES(benchmark::Bench& bench)
{
    CHACHA20(bench, ChaCha20Aligned::BLOCKLEN * 12);
}

static void CHACHA20_832BYTES(benchmark::Bench& bench)
{
    CHACHA20(bench, ChaCha20Aligned::BLOCKLEN * 13);
}

static void CHACHA20_896BYTES(benchmark::Bench& bench)
{
    CHACHA20(bench, ChaCha20Aligned::BLOCKLEN * 14);
}

static void CHACHA20_960BYTES(benchmark::Bench& bench)
{
    CHACHA20(bench, ChaCha20Aligned::BLOCKLEN * 15);
}

static void CHACHA20_1MB(benchmark::Bench& bench)
{
    CHACHA20(bench, BUFFER_SIZE_LARGE);
}

static void FSCHACHA20POLY1305_64BYTES(benchmark::Bench& bench)
{
    FSCHACHA20POLY1305(bench, BUFFER_SIZE_TINY);
}

static void FSCHACHA20POLY1305_256BYTES(benchmark::Bench& bench)
{
    FSCHACHA20POLY1305(bench, BUFFER_SIZE_SMALL);
}

static void FSCHACHA20POLY1305_BIP324_318BYTES(benchmark::Bench& bench)
{
    FSCHACHA20POLY1305(bench, 318);
}

static void FSCHACHA20POLY1305_BIP324_319BYTES(benchmark::Bench& bench)
{
    FSCHACHA20POLY1305(bench, 319);
}

static void FSCHACHA20POLY1305_BIP324_320BYTES(benchmark::Bench& bench)
{
    FSCHACHA20POLY1305(bench, 320);
}

static void FSCHACHA20POLY1305_1MB(benchmark::Bench& bench)
{
    FSCHACHA20POLY1305(bench, BUFFER_SIZE_LARGE);
}

BENCHMARK(CHACHA20_64BYTES);
BENCHMARK(CHACHA20_128BYTES);
BENCHMARK(CHACHA20_192BYTES);
BENCHMARK(CHACHA20_256BYTES);
BENCHMARK(CHACHA20_768BYTES);
BENCHMARK(CHACHA20_832BYTES);
BENCHMARK(CHACHA20_896BYTES);
BENCHMARK(CHACHA20_960BYTES);
BENCHMARK(CHACHA20_1MB);
BENCHMARK(FSCHACHA20POLY1305_64BYTES);
BENCHMARK(FSCHACHA20POLY1305_256BYTES);
BENCHMARK(FSCHACHA20POLY1305_BIP324_318BYTES);
BENCHMARK(FSCHACHA20POLY1305_BIP324_319BYTES);
BENCHMARK(FSCHACHA20POLY1305_BIP324_320BYTES);
BENCHMARK(FSCHACHA20POLY1305_1MB);
