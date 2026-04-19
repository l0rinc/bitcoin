// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <bench/bench.h>
#include <crypto/chacha20.h>
#include <crypto/chacha20poly1305.h>
#include <span.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

/* Number of bytes to process per iteration */
static const uint64_t BUFFER_SIZE_TINY  = 64;
static const uint64_t BUFFER_SIZE_SMALL = 256;
static const uint64_t BUFFER_SIZE_LARGE = 1024*1024;

static void CHACHA20(benchmark::Bench& bench, size_t buffersize)
{
    std::vector<std::byte> key(32, {});
    std::optional<ChaCha20> ctx;
    std::vector<std::byte> in(buffersize, {});
    std::vector<std::byte> out(buffersize, {});
    bench.batch(in.size()).unit("byte").epochIterations(1)
        .setup([&] { ctx.emplace(key); })
        .run([&] {
            ctx->Crypt(in, out);
            assert(out[0] == std::byte{0x76});
        });
}

static void FSCHACHA20POLY1305(benchmark::Bench& bench, size_t buffersize)
{
    std::vector<std::byte> key(32);
    std::optional<FSChaCha20Poly1305> ctx;
    std::vector<std::byte> in(buffersize);
    std::vector<std::byte> aad;
    std::vector<std::byte> out(buffersize + FSChaCha20Poly1305::EXPANSION);
    bench.batch(in.size()).unit("byte").epochIterations(1)
        .setup([&] { ctx.emplace(key, 224); })
        .run([&] {
            ctx->Encrypt(in, aad, out);
            assert(out[0] == std::byte{0x9f});
        });
}

static void CHACHA20_64BYTES(benchmark::Bench& bench)
{
    CHACHA20(bench, BUFFER_SIZE_TINY);
}

static void CHACHA20_256BYTES(benchmark::Bench& bench)
{
    CHACHA20(bench, BUFFER_SIZE_SMALL);
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

static void FSCHACHA20POLY1305_1MB(benchmark::Bench& bench)
{
    FSCHACHA20POLY1305(bench, BUFFER_SIZE_LARGE);
}

BENCHMARK(CHACHA20_64BYTES);
BENCHMARK(CHACHA20_256BYTES);
BENCHMARK(CHACHA20_1MB);
BENCHMARK(FSCHACHA20POLY1305_64BYTES);
BENCHMARK(FSCHACHA20POLY1305_256BYTES);
BENCHMARK(FSCHACHA20POLY1305_1MB);
