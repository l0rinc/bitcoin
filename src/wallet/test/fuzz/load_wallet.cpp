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
#include <algorithm>
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
    std::vector<unsigned int> seeded_mkey_ids;
    bool seeded_dup_mkey{false};

    {
        auto batch{database->MakeBatch()};
        LIMITED_WHILE(fdp.ConsumeBool(), 24) {
            switch (fdp.ConsumeIntegralInRange<int>(0, 11)) {
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
            case 6: { // CRYPTED_KEY record: pubkey key + mutated crypted payload
                const auto pubkey_bytes{ConsumeRandomLengthByteVector(fdp, 33)};
                (void)batch->Write(std::make_pair(DBKeys::CRYPTED_KEY, pubkey_bytes), ConsumeRandomLengthByteVector(fdp, 64));
                break;
            }
            case 7: { // ACTIVE*SPK records: output-type + descriptor id
                const uint8_t output_type{fdp.ConsumeIntegralInRange<uint8_t>(0, 7)};
                const uint256 id{ConsumeUInt256(fdp)};
                // LoadActiveScriptPubKeyMan asserts WALLET_FLAG_DESCRIPTORS
                // (wallet.cpp:3757): a corrupt DB with ACTIVE*SPK records but
                // no DESCRIPTORS flag aborts there instead of yielding a
                // DBErrors classification. That is upstream's design choice
                // for the invariant (upstream-matching, corrupt-local-DB
                // only) — recorded as the known-reachable assertion, and the
                // combination is gated here so the fuzzer can progress past it.
                if (seeded_flags.has_value() && (*seeded_flags & WALLET_FLAG_DESCRIPTORS) != 0) {
                    (void)batch->Write(std::make_pair(DBKeys::ACTIVEEXTERNALSPK, output_type), id);
                    (void)batch->Write(std::make_pair(DBKeys::ACTIVEINTERNALSPK, output_type), id);
                }
                break;
            }
            case 8: { // BESTBLOCK record: mutated locator bytes
                (void)batch->Write(DBKeys::BESTBLOCK, ConsumeRandomLengthByteVector(fdp, 256));
                break;
            }
            case 9: { // MASTER_KEY record: id + fuzzed CMasterKey fields
                const unsigned int mkey_id{fdp.ConsumeIntegral<unsigned int>() % 4}; // small range to make duplicates likely
                if (std::find(seeded_mkey_ids.begin(), seeded_mkey_ids.end(), mkey_id) != seeded_mkey_ids.end()) {
                    seeded_dup_mkey = true;
                }
                seeded_mkey_ids.push_back(mkey_id);
                CMasterKey mkey;
                mkey.vchCryptedKey = ConsumeRandomLengthByteVector(fdp, 32);
                mkey.vchSalt = ConsumeRandomLengthByteVector(fdp, 8);
                mkey.nDerivationMethod = fdp.ConsumeIntegral<unsigned int>() % 3;
                mkey.nDeriveIterations = fdp.ConsumeIntegral<unsigned int>();
                mkey.vchOtherDerivationParameters = ConsumeRandomLengthByteVector(fdp, 16);
                // Duplicate-id MASTER_KEY records must classify DBErrors::CORRUPT
                // (walletdb.cpp:403-407) — asserted after LoadWallet.
                (void)batch->Write(std::make_pair(DBKeys::MASTER_KEY, mkey_id), mkey);
                break;
            }
            case 10: { // HDCHAIN record: fuzzed CHDChain (version/counters/seed id)
                CHDChain hd;
                hd.nVersion = fdp.PickValueInArray<int>({0, 1, 2, 3, 99});
                hd.nExternalChainCounter = fdp.ConsumeIntegral<uint32_t>();
                hd.nInternalChainCounter = fdp.ConsumeIntegral<uint32_t>();
                // ConsumeRandomLengthByteVector may return fewer bytes than
                // requested; uint160's span ctor asserts the exact width.
                auto seed_bytes{ConsumeRandomLengthByteVector(fdp, 20)};
                seed_bytes.resize(20, 0);
                hd.seed_id = CKeyID{uint160{seed_bytes}};
                (void)batch->Write(DBKeys::HDCHAIN, hd);
                break;
            }
            case 11: { // KEYMETA record: pubkey + fuzzed CKeyMetadata
                std::vector<unsigned char> pubkey_bytes;
                if (fdp.ConsumeBool()) {
                    const CKey key{ConsumePrivateKey(fdp)};
                    if (key.IsValid()) {
                        const CPubKey pubkey{key.GetPubKey()};
                        pubkey_bytes = std::vector<unsigned char>(pubkey.begin(), pubkey.end());
                    }
                }
                if (pubkey_bytes.empty()) pubkey_bytes = ConsumeRandomLengthByteVector(fdp, 33);
                CKeyMetadata meta;
                meta.nVersion = fdp.PickValueInArray<int>({1, 10, 12, 99});
                meta.nCreateTime = fdp.ConsumeIntegral<int64_t>();
                meta.hdKeypath = fdp.ConsumeRandomLengthString(32);
                auto meta_seed_bytes{ConsumeRandomLengthByteVector(fdp, 20)};
                meta_seed_bytes.resize(20, 0);
                meta.hd_seed_id = CKeyID{uint160{meta_seed_bytes}};
                for (auto& b : meta.key_origin.fingerprint) b = fdp.ConsumeIntegral<uint8_t>();
                LIMITED_WHILE(fdp.ConsumeBool(), 8) {
                    meta.key_origin.path.push_back(fdp.ConsumeIntegral<uint32_t>());
                }
                meta.has_key_origin = fdp.ConsumeBool();
                (void)batch->Write(std::make_pair(DBKeys::KEYMETA, pubkey_bytes), meta);
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

    // Duplicate MASTER_KEY ids must classify DBErrors::CORRUPT
    // (LoadEncryptionKey's duplicate check). Deterministic, input-driven.
    if (seeded_dup_mkey) assert(result == DBErrors::CORRUPT);

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
