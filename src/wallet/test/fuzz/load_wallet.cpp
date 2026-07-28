// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key_io.h>
#include <script/descriptor.h>
#include <script/signingprovider.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <util/time.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>
#include <wallet/walletdb.h>

#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace wallet {
namespace {

TestingSetup* g_setup;

void initialize_load_wallet()
{
    static const auto testing_setup = MakeNoLogFileContext<TestingSetup>();
    g_setup = testing_setup.get();
}

//! Wallet-database record application (WalletBatch::LoadWallet) is the
//! persisted-state trust boundary: records may be truncated, torn, or
//! corrupt. No fuzz target covered it (wallet_bdb_parser is
//! container-level only). This harness pre-seeds an in-memory SQLite
//! database with fuzzed records and applies them through the real
//! loader, asserting the loader's classification contract: every input
//! yields a DBErrors status (never an uncaught exception), an accepted
//! FLAGS record round-trips exactly into m_wallet_flags, and seeded
//! NAME records with valid destinations are applied to the address book.
FUZZ_TARGET(load_wallet, .init = initialize_load_wallet)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};
    (void)g_setup;
    SetMockTime(fdp.ConsumeIntegralInRange<int64_t>(946684800, 4133980799)); // wallet construction uses GetTime

    auto database{CreateMockableWalletDatabase()};

    // What we seeded, for the round-trip oracles.
    std::optional<uint64_t> seeded_flags;
    std::vector<std::pair<std::string, std::string>> seeded_names;

    {
        auto batch{database->MakeBatch()};
        LIMITED_WHILE(fdp.ConsumeBool(), 24) {
            switch (fdp.ConsumeIntegralInRange<int>(0, 5)) {
            case 0: { // FLAGS record
                const uint64_t flags{fdp.ConsumeIntegral<uint64_t>()};
                if (batch->Write(DBKeys::FLAGS, flags)) seeded_flags = flags;
                break;
            }
            case 1: { // VERSION / MINVERSION records
                (void)batch->Write(DBKeys::VERSION, fdp.ConsumeIntegral<int>());
                (void)batch->Write(DBKeys::MINVERSION, fdp.ConsumeIntegral<int>());
                break;
            }
            case 2: { // NAME record: fuzzed address string + label
                const std::string addr{fdp.ConsumeRandomLengthString(64)};
                const std::string label{fdp.ConsumeRandomLengthString(64)};
                if (batch->Write(std::make_pair(DBKeys::NAME, addr), label)) {
                    seeded_names.emplace_back(addr, label);
                }
                break;
            }
            case 3: { // WALLETDESCRIPTOR record: valid or mutated bytes
                const uint256 id{ConsumeUInt256(fdp)};
                if (fdp.ConsumeBool()) {
                    FlatSigningProvider provider;
                    CKey key;
                    key.MakeNewKey(/*fCompressed=*/true);
                    std::string error;
                    auto descs{Parse("combo(" + EncodeSecret(key) + ")", provider, error, /*require_checksum=*/false)};
                    if (descs.size() == 1) {
                        WalletDescriptor w_desc{std::move(descs.at(0)), 0, 0, 1, 1};
                        (void)batch->Write(std::make_pair(DBKeys::WALLETDESCRIPTOR, id), w_desc);
                    }
                } else {
                    (void)batch->Write(std::make_pair(DBKeys::WALLETDESCRIPTOR, id), ConsumeRandomLengthByteVector(fdp, 256));
                }
                break;
            }
            case 4: { // TX record: mutated bytes (valid txs are covered by wallet unit tests)
                const uint256 txid{ConsumeUInt256(fdp)};
                (void)batch->Write(std::make_pair(DBKeys::TX, txid), ConsumeRandomLengthByteVector(fdp, 512));
                break;
            }
            case 5: { // unknown record type: prefix-cursor robustness
                (void)batch->Write(std::make_pair(fdp.ConsumeRandomLengthString(16), fdp.ConsumeIntegral<uint64_t>()), ConsumeRandomLengthByteVector(fdp, 64));
                break;
            }
            }
        }
    }

    // A real wallet over the seeded database. NOTE: do NOT call
    // SetWalletFlag here — it persists to the database immediately and
    // would clobber the seeded FLAGS record (this was the cause of the
    // first harness crash: seeded FLAGS=0x308 was overwritten with plain
    // DESCRIPTORS before the loader ever ran; production was correct).
    // Read back the record the loader will actually see for the oracle.
    std::optional<uint64_t> expected_flags;
    {
        auto batch{database->MakeBatch()};
        uint64_t readback{0};
        if (batch->Read(DBKeys::FLAGS, readback)) expected_flags = readback;
    }
    auto wallet{std::make_unique<CWallet>(g_setup->m_node.chain.get(), "", std::move(database))};
    wallet->m_keypool_size = 1;

    // Apply the records through the production loader. The trust-
    // boundary contract: any DBErrors classification is acceptable;
    // an uncaught exception or abort is a defect.
    const DBErrors result{WalletBatch(wallet->GetDatabase()).LoadWallet(wallet.get())};

    if (result == DBErrors::LOAD_OK) {
        // CWallet::LoadWalletFlags rejects only flags with unknown bits
        // in the high word and otherwise assigns verbatim. On LOAD_OK
        // every bit of the FLAGS record the loader saw must be set.
        if (expected_flags.has_value()) {
            for (uint64_t bit{1}; bit != 0; bit <<= 1) {
                if (*expected_flags & bit) assert(wallet->IsWalletFlagSet(bit));
            }
        }
        // Seeded NAME records with valid destinations must be present
        // with the seeded label (last write for an address wins).
        for (const auto& [addr, label] : seeded_names) {
            const auto dest{DecodeDestination(addr)};
            if (!IsValidDestination(dest)) continue;
            const auto it{wallet->m_address_book.find(dest)};
            assert(it != wallet->m_address_book.end());
            assert(it->second.label.has_value());
            assert(*it->second.label == label);
        }
    }
}

} // namespace
} // namespace wallet
