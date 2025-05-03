// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <swiftsync.h>
#include <streams.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <util/fs.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(swiftsync_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(bitmap_roundtrip)
{
    const fs::path path{m_args.GetDataDirBase() / "swiftsync_bitmap_test.dat"};
    constexpr uint32_t NUM_BLOCKS{10};

    std::vector<std::vector<bool>> original(NUM_BLOCKS);

    // --- write file --------------------------------------------------------
    {
        AutoFile f{fsbridge::fopen(path, "wb")};

        for (uint32_t h{0}; h < NUM_BLOCKS; ++h) {
            const uint16_t bits = 1 + m_rng.randrange(200);
            original[h].resize(bits);

            f << bits;

            for (uint16_t i = 0; i < bits; i += 8) {
                uint8_t byte{0};
                for (int j = 0; j < 8 && i + j < bits; ++j) {
                    const bool val = m_rng.randbool();
                    original[h][i + j] = val;
                    if (val) byte |= 1 << j;
                }
                f << byte;
            }
        }

        uint16_t end_marker = 0;
        f << end_marker;
    }

    // --- read & verify -----------------------------------------------------
    {
        SwiftSyncHints hints;
        hints.Load(fs::PathToString(path));

        BOOST_CHECK(hints.IsLoaded());
        BOOST_CHECK_EQUAL(hints.GetTerminalBlockHeight(), NUM_BLOCKS - 1);

        for (uint32_t h{0}; h < NUM_BLOCKS; ++h) {
            hints.SetCurrentBlockHeight(h);
            for (size_t i = 0; i < original[h].size(); ++i) {
                BOOST_CHECK_EQUAL(hints.GetNextBit(), original[h][i]);
            }
            // After the expected number of bits a further call MUST throw.
            BOOST_CHECK_THROW(hints.GetNextBit(), std::out_of_range);
        }
    }

    fs::remove(path);
}

BOOST_AUTO_TEST_SUITE_END()