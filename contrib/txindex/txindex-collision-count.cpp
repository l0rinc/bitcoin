// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <leveldb/db.h>
#include <leveldb/env.h>
#include <leveldb/iterator.h>
#include <leveldb/options.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <string>

namespace {
constexpr char HASHED_ROW{'x'};
constexpr size_t HASH_PREFIX_BYTES{5};
constexpr size_t GROUP_BYTES{1 + HASH_PREFIX_BYTES};
constexpr size_t HASHED_KEY_BYTES{GROUP_BYTES + 3 + 3};
constexpr uint64_t PREFIX_BUCKETS{uint64_t{1} << (8 * HASH_PREFIX_BYTES)};

class SilentLogger final : public leveldb::Logger
{
public:
    void Logv(const char*, std::va_list) override {}
};

void AddBucket(uint64_t size,
               std::map<uint64_t, uint64_t>& histogram,
               uint64_t& colliding_buckets,
               uint64_t& extra_rows,
               uint64_t& colliding_pairs,
               uint64_t& largest_bucket)
{
    if (size == 0) return;
    ++histogram[size];
    if (size > 1) {
        ++colliding_buckets;
        extra_rows += size - 1;
        colliding_pairs += size * (size - 1) / 2;
    }
    largest_bucket = std::max(largest_bucket, size);
}
} // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " TXINDEX_LEVELDB_PATH\n";
        return 1;
    }

    SilentLogger logger;
    leveldb::Options options;
    options.create_if_missing = false;
    options.paranoid_checks = true;
    options.info_log = &logger;

    leveldb::DB* raw_db{nullptr};
    const leveldb::Status open_status{leveldb::DB::Open(options, argv[1], &raw_db)};
    if (!open_status.ok()) {
        std::cerr << "Unable to open txindex: " << open_status.ToString() << '\n';
        return 1;
    }
    const std::unique_ptr<leveldb::DB> db{raw_db};

    leveldb::ReadOptions read_options;
    read_options.verify_checksums = true;
    read_options.fill_cache = false;
    const std::unique_ptr<leveldb::Iterator> it{db->NewIterator(read_options)};

    std::array<char, GROUP_BYTES> current_group{};
    bool have_group{false};
    uint64_t current_size{0};
    uint64_t hashed_rows{0};
    uint64_t malformed_rows{0};
    uint64_t colliding_buckets{0};
    uint64_t extra_rows{0};
    uint64_t colliding_pairs{0};
    uint64_t largest_bucket{0};
    std::map<uint64_t, uint64_t> histogram;
    const auto start{std::chrono::steady_clock::now()};

    for (it->Seek(leveldb::Slice{&HASHED_ROW, 1}); it->Valid(); it->Next()) {
        const leveldb::Slice key{it->key()};
        if (key.empty() || key[0] != HASHED_ROW) break;
        if (key.size() != HASHED_KEY_BYTES) {
            ++malformed_rows;
            continue;
        }

        std::array<char, GROUP_BYTES> group;
        std::memcpy(group.data(), key.data(), group.size());
        if (!have_group || group != current_group) {
            AddBucket(current_size, histogram, colliding_buckets, extra_rows,
                      colliding_pairs, largest_bucket);
            current_group = group;
            current_size = 0;
            have_group = true;
        }
        ++current_size;
        ++hashed_rows;
        if (hashed_rows % 100'000'000 == 0) {
            std::cerr << "Scanned " << hashed_rows << " hashed rows\n";
        }
    }
    if (!it->status().ok()) {
        std::cerr << "Iterator error: " << it->status().ToString() << '\n';
        return 1;
    }
    AddBucket(current_size, histogram, colliding_buckets, extra_rows,
              colliding_pairs, largest_bucket);
    if (hashed_rows == 0) {
        std::cerr << "No hashed txindex rows found\n";
        return 1;
    }

    uint64_t total_buckets{0};
    for (const auto& [size, buckets] : histogram)
        total_buckets += buckets;

    const long double lambda{
        static_cast<long double>(hashed_rows) / static_cast<long double>(PREFIX_BUCKETS)};
    const long double expected_pairs{
        static_cast<long double>(hashed_rows) * (hashed_rows - 1) /
        (2 * static_cast<long double>(PREFIX_BUCKETS))};
    const long double expected_two{
        PREFIX_BUCKETS * std::exp(-lambda) * std::pow(lambda, 2) / 2};
    const long double expected_three{
        PREFIX_BUCKETS * std::exp(-lambda) * std::pow(lambda, 3) / 6};
    const uint64_t observed_two{histogram.contains(2) ? histogram.at(2) : 0};
    const uint64_t observed_three{histogram.contains(3) ? histogram.at(3) : 0};
    const long double successful_lookup_amplification{
        1 + static_cast<long double>(colliding_pairs) / hashed_rows};
    const std::chrono::duration<double> elapsed{
        std::chrono::steady_clock::now() - start};

    std::cout << "txindex: " << argv[1] << '\n'
              << "hashed rows: " << hashed_rows << '\n'
              << "hash-prefix buckets: " << total_buckets << '\n'
              << "colliding buckets: " << colliding_buckets << '\n'
              << "extra rows sharing a prefix: " << extra_rows << '\n'
              << "colliding pairs: " << colliding_pairs << '\n'
              << "largest bucket: " << largest_bucket << '\n'
              << "malformed x rows: " << malformed_rows << '\n'
              << std::fixed << std::setprecision(2)
              << "expected colliding pairs: " << expected_pairs << '\n'
              << "observed exactly-2 buckets: " << observed_two << '\n'
              << "expected exactly-2 buckets: " << expected_two << '\n'
              << "observed exactly-3 buckets: " << observed_three << '\n'
              << "expected exactly-3 buckets: " << expected_three << '\n'
              << std::setprecision(6)
              << "estimated uniform successful-lookup amplification: "
              << successful_lookup_amplification << "x\n"
              << "expected candidates for a uniform missing lookup: " << lambda << '\n'
              << std::setprecision(2)
              << "scan seconds: " << elapsed.count() << '\n'
              << "bucket size\tbuckets\trows\n";
    for (const auto& [size, buckets] : histogram) {
        std::cout << size << '\t' << buckets << '\t' << size * buckets << '\n';
    }
    std::cout << std::setprecision(4)
              << "Conclusion: collisions add about "
              << 100 * (successful_lookup_amplification - 1)
              << "% estimated candidate reads to uniform successful lookups; "
              << "three-entry buckets: " << observed_three << " observed, "
              << expected_three << " expected.\n";
    return 0;
}
