// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <chainparams.h>
#include <coins.h>
#include <key.h>
#include <primitives/transaction.h>
#include <psbt.h>
#include <script/descriptor.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/descriptor.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/check.h>
#include <util/time.h>
#include <util/translation.h>
#include <util/string.h>
#include <validation.h>
#include <wallet/context.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/test/util.h>
#include <wallet/types.h>
#include <wallet/wallet.h>
#include <wallet/walletdb.h>
#include <wallet/walletutil.h>

#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <sqlite3.h>
#include <streams.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace wallet {
namespace {
const TestingSetup* g_setup;

//! The converter of mocked descriptors, needs to be initialized when the target is.
MockedDescriptorConverter MOCKED_DESC_CONVERTER;

void initialize_spkm()
{
    static const auto testing_setup{MakeNoLogFileContext<const TestingSetup>()};
    g_setup = testing_setup.get();
    MOCKED_DESC_CONVERTER.Init();
}

void initialize_spkm_migration()
{
    static const auto testing_setup{MakeNoLogFileContext<const TestingSetup>()};
    g_setup = testing_setup.get();
}

static std::optional<std::pair<WalletDescriptor, FlatSigningProvider>> CreateWalletDescriptor(FuzzedDataProvider& fuzzed_data_provider)
{
    const std::string mocked_descriptor{fuzzed_data_provider.ConsumeRandomLengthString()};
    const auto desc_str{MOCKED_DESC_CONVERTER.GetDescriptor(mocked_descriptor)};
    if (!desc_str.has_value()) return std::nullopt;
    if (IsTooExpensive(MakeUCharSpan(*desc_str))) return {};

    FlatSigningProvider keys;
    std::string error;
    std::vector<std::unique_ptr<Descriptor>> parsed_descs = Parse(desc_str.value(), keys, error, false);
    if (parsed_descs.empty()) return std::nullopt;

    // Verify expand succeeds before making WalletDescriptor
    // Expansion results are not needed
    FlatSigningProvider out_keys;
    std::vector<CScript> scripts_temp;
    DescriptorCache temp_cache;
    if (!parsed_descs.at(0)->Expand(0, keys, scripts_temp, out_keys, &temp_cache)) return std::nullopt;

    WalletDescriptor w_desc{std::move(parsed_descs.at(0)), /*creation_time=*/0, /*range_start=*/0, /*range_end=*/1, /*next_index=*/1};
    return std::make_pair(w_desc, keys);
}

static DescriptorScriptPubKeyMan* CreateDescriptor(WalletDescriptor& wallet_desc, FlatSigningProvider& keys, CWallet& keystore)
{
    LOCK(keystore.cs_wallet);
    auto spk_manager_res = keystore.AddWalletDescriptor(wallet_desc, keys, /*label=*/"", /*internal=*/false);
    if (!spk_manager_res) return nullptr;
    return &spk_manager_res.value().get();
};

static bool BlockDescriptorWrites(CWallet& wallet)
{
    auto* database{dynamic_cast<MockableSQLiteDatabase*>(&wallet.GetDatabase())};
    return database && sqlite3_exec(database->m_db,
                                    "CREATE TRIGGER fail_wallet_writes BEFORE INSERT ON main BEGIN SELECT RAISE(ABORT, 'injected'); END;",
                                    nullptr, nullptr, nullptr) == SQLITE_OK;
}

static bool BlockDescriptorMetadataWrites(CWallet& wallet)
{
    auto* database{dynamic_cast<MockableSQLiteDatabase*>(&wallet.GetDatabase())};
    // The serialized SQLite keys are 49 bytes for descriptor metadata and 62 bytes for cache entries.
    return database && sqlite3_exec(database->m_db,
                                    "CREATE TRIGGER fail_descriptor_metadata_writes BEFORE INSERT ON main WHEN length(NEW.key) = 49 BEGIN SELECT RAISE(ABORT, 'injected'); END;",
                                    nullptr, nullptr, nullptr) == SQLITE_OK;
}

static std::optional<WalletDescriptor> ReadPersistedDescriptor(CWallet& wallet, const uint256& id)
{
    auto* database{dynamic_cast<MockableSQLiteDatabase*>(&wallet.GetDatabase())};
    if (!database) return std::nullopt;
    DataStream key;
    key << std::make_pair(DBKeys::WALLETDESCRIPTOR, id);
    sqlite3_stmt* statement{nullptr};
    if (sqlite3_prepare_v2(database->m_db, "SELECT value FROM main WHERE key = ?", -1, &statement, nullptr) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return std::nullopt;
    }
    if (sqlite3_bind_blob(statement, 1, static_cast<const void*>(key.data()), key.size(), SQLITE_TRANSIENT) != SQLITE_OK ||
        sqlite3_step(statement) != SQLITE_ROW) {
        sqlite3_finalize(statement);
        return std::nullopt;
    }
    SpanReader descriptor_reader{std::span<const std::byte>{static_cast<const std::byte*>(sqlite3_column_blob(statement, 0)), static_cast<size_t>(sqlite3_column_bytes(statement, 0))}};
    WalletDescriptor descriptor;
    descriptor_reader >> descriptor;
    const bool unique{sqlite3_step(statement) == SQLITE_DONE};
    sqlite3_finalize(statement);
    if (!unique) return std::nullopt;
    return descriptor;
}

FUZZ_TARGET(scriptpubkeyman, .init = initialize_spkm)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};
    const auto& node{g_setup->m_node};
    Chainstate& chainstate{node.chainman->ActiveChainstate()};
    std::unique_ptr<CWallet> wallet_ptr{std::make_unique<CWallet>(node.chain.get(), "", CreateMockableWalletDatabase())};
    CWallet& wallet{*wallet_ptr};
    {
        LOCK(wallet.cs_wallet);
        wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        wallet.SetLastBlockProcessed(chainstate.m_chain.Height(), chainstate.m_chain.Tip()->GetBlockHash());
        wallet.m_keypool_size = 1;
    }

    auto wallet_desc{CreateWalletDescriptor(fuzzed_data_provider)};
    if (!wallet_desc.has_value()) return;
    auto spk_manager{CreateDescriptor(wallet_desc->first, wallet_desc->second, wallet)};
    if (spk_manager == nullptr) return;

    if (fuzzed_data_provider.ConsumeBool()) {
        auto wallet_desc{CreateWalletDescriptor(fuzzed_data_provider)};
        if (!wallet_desc.has_value()) {
            return;
        }
        std::string error;
        if (spk_manager->CanUpdateToWalletDescriptor(wallet_desc->first, error)) {
            auto new_spk_manager{CreateDescriptor(wallet_desc->first, wallet_desc->second, wallet)};
            if (new_spk_manager != nullptr) spk_manager = new_spk_manager;
        }
    }

    bool good_data{true};
    bool descriptor_writes_blocked{false};
    bool descriptor_metadata_writes_blocked{false};
    LIMITED_WHILE (good_data && fuzzed_data_provider.ConsumeBool(), 20) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                const CScript script{ConsumeScript(fuzzed_data_provider)};
                if (spk_manager->IsMine(script)) {
                    assert(spk_manager->GetScriptPubKeys().contains(script));
                }
            },
            [&] {
                auto spks{spk_manager->GetScriptPubKeys()};
                for (const CScript& spk : spks) {
                    assert(spk_manager->IsMine(spk));
                    CTxDestination dest;
                    bool extract_dest{ExtractDestination(spk, dest)};
                    if (extract_dest) {
                        const std::string msg{fuzzed_data_provider.ConsumeRandomLengthString()};
                        PKHash pk_hash{std::get_if<PKHash>(&dest) && fuzzed_data_provider.ConsumeBool() ?
                                           *std::get_if<PKHash>(&dest) :
                                           PKHash{ConsumeUInt160(fuzzed_data_provider)}};
                        std::string str_sig;
                        (void)spk_manager->SignMessage(msg, pk_hash, str_sig);
                        (void)spk_manager->GetMetadata(dest);
                    }
                }
            },
            [&] {
                auto spks{spk_manager->GetScriptPubKeys()};
                if (!spks.empty()) {
                    auto& spk{PickValue(fuzzed_data_provider, spks)};
                    (void)spk_manager->MarkUnusedAddresses(spk);
                }
            },
            [&] {
                if (descriptor_writes_blocked || descriptor_metadata_writes_blocked) return;
                LOCK(spk_manager->cs_desc_man);
                const auto descriptor_before_new{spk_manager->GetWalletDescriptor()};
                const auto output_type{descriptor_before_new.descriptor->GetOutputType()};
                if (!descriptor_before_new.descriptor->IsSingleType() ||
                    !descriptor_before_new.descriptor->IsRange() ||
                    !output_type.has_value() ||
                    descriptor_before_new.next_index >= std::numeric_limits<int32_t>::max() - 10) {
                    return;
                }

                if (!spk_manager->GetNewDestination(*output_type) || !spk_manager->TopUp(10)) return;
                const auto descriptor_before_mark{spk_manager->GetWalletDescriptor()};
                FlatSigningProvider out_keys;
                std::vector<CScript> scripts;
                if (!descriptor_before_mark.descriptor->ExpandFromCache(descriptor_before_mark.next_index,
                                                                         descriptor_before_mark.cache,
                                                                         scripts, out_keys) ||
                    scripts.size() != 1) {
                    return;
                }

                const auto marked{spk_manager->MarkUnusedAddresses(scripts.front())};
                assert(marked.size() == 1);
                const auto descriptor_after_mark{spk_manager->GetWalletDescriptor()};
                assert(descriptor_after_mark.next_index == descriptor_before_mark.next_index + 1);
                const auto persisted{ReadPersistedDescriptor(wallet, spk_manager->GetID())};
                assert(persisted && persisted->next_index == descriptor_after_mark.next_index);
            },
            [&] {
                if (descriptor_writes_blocked || descriptor_metadata_writes_blocked) return;
                LOCK(spk_manager->cs_desc_man);
                const auto descriptor_before_new{spk_manager->GetWalletDescriptor()};
                const auto output_type{descriptor_before_new.descriptor->GetOutputType()};
                if (!descriptor_before_new.descriptor->IsSingleType() ||
                    !descriptor_before_new.descriptor->IsRange() ||
                    !output_type.has_value() ||
                    descriptor_before_new.next_index >= std::numeric_limits<int32_t>::max() - 10) {
                    return;
                }

                if (!spk_manager->GetNewDestination(*output_type) || !spk_manager->TopUp(10)) return;
                const auto descriptor_before_mark{spk_manager->GetWalletDescriptor()};
                FlatSigningProvider out_keys;
                std::vector<CScript> scripts;
                if (!descriptor_before_mark.descriptor->ExpandFromCache(descriptor_before_mark.next_index,
                                                                         descriptor_before_mark.cache,
                                                                         scripts, out_keys) ||
                    scripts.size() != 1) {
                    return;
                }

                if (!BlockDescriptorMetadataWrites(wallet)) {
                    good_data = false;
                    return;
                }
                descriptor_metadata_writes_blocked = true;

                bool expected_failure{false};
                try {
                    (void)spk_manager->MarkUnusedAddresses(scripts.front());
                } catch (const std::runtime_error& error) {
                    if (std::string_view{error.what()} != "TopUpWithDB: writing descriptor failed") {
                        throw;
                    }
                    expected_failure = true;
                }
                assert(expected_failure);
                const auto descriptor_after_failure{spk_manager->GetWalletDescriptor()};
                assert(descriptor_after_failure.next_index == descriptor_before_mark.next_index);
                const auto persisted{ReadPersistedDescriptor(wallet, spk_manager->GetID())};
                assert(persisted && persisted->next_index == descriptor_before_mark.next_index);
            },
            [&] {
                LOCK(spk_manager->cs_desc_man);
                auto wallet_desc{spk_manager->GetWalletDescriptor()};
                if (wallet_desc.descriptor->IsSingleType()) {
                    auto output_type{wallet_desc.descriptor->GetOutputType()};
                    if (output_type.has_value()) {
                        auto dest{spk_manager->GetNewDestination(*output_type)};
                        if (dest) {
                            assert(IsValidDestination(*dest));
                            assert(spk_manager->IsHDEnabled());
                        }
                    }
                }
            },
            [&] {
                if (descriptor_writes_blocked || descriptor_metadata_writes_blocked) return;
                LOCK(spk_manager->cs_desc_man);
                auto wallet_desc{spk_manager->GetWalletDescriptor()};
                const auto output_type{wallet_desc.descriptor->GetOutputType()};
                if (!wallet_desc.descriptor->IsSingleType() ||
                    !wallet_desc.descriptor->IsRange() ||
                    !wallet_desc.descriptor->CanSelfExpand() ||
                    !output_type.has_value()) {
                    return;
                }

                // Consume the current cache so the next request must rely on self-expansion.
                while (wallet_desc.next_index < wallet_desc.range_end) {
                    const auto dest{spk_manager->GetNewDestination(*output_type)};
                    assert(dest && IsValidDestination(*dest));
                    wallet_desc = spk_manager->GetWalletDescriptor();
                }
                assert(wallet_desc.next_index == wallet_desc.range_end);
                assert(spk_manager->CanGetAddresses());

                const auto dest{spk_manager->GetNewDestination(*output_type)};
                assert(dest && IsValidDestination(*dest));
                const auto descriptor_after{spk_manager->GetWalletDescriptor()};
                assert(descriptor_after.next_index == wallet_desc.next_index + 1);
                assert(descriptor_after.range_end > wallet_desc.range_end);
            },
            [&] {
                if (descriptor_writes_blocked || descriptor_metadata_writes_blocked) return;
                LOCK(spk_manager->cs_desc_man);
                auto wallet_desc{spk_manager->GetWalletDescriptor()};
                const auto output_type{wallet_desc.descriptor->GetOutputType()};
                if (!wallet_desc.descriptor->IsSingleType() || !output_type.has_value()) return;

                int64_t reserved_index{-1};
                const auto reserved{spk_manager->GetReservedDestination(*output_type, /*internal=*/false, reserved_index)};
                if (!reserved) return;
                const auto reserved_desc{spk_manager->GetWalletDescriptor()};
                if (!BlockDescriptorWrites(wallet)) {
                    good_data = false;
                    return;
                }
                descriptor_writes_blocked = true;

                spk_manager->ReturnDestination(reserved_index, /*internal=*/false, *reserved);
                // A failed descriptor write must not publish the returned index in memory.
                assert(spk_manager->GetWalletDescriptor().next_index == reserved_desc.next_index);
            },
            [&] {
                if (descriptor_writes_blocked || descriptor_metadata_writes_blocked) return;
                if (!BlockDescriptorWrites(wallet)) {
                    good_data = false;
                    return;
                }
                descriptor_writes_blocked = true;

                LOCK(spk_manager->cs_desc_man);
                auto wallet_desc{spk_manager->GetWalletDescriptor()};
                if (!wallet_desc.descriptor->IsSingleType()) return;
                const auto output_type{wallet_desc.descriptor->GetOutputType()};
                if (!output_type.has_value()) return;
                const int32_t next_index{wallet_desc.next_index};
                try {
                    assert(!spk_manager->GetNewDestination(*output_type));
                } catch (const std::runtime_error& error) {
                    const std::string_view message{error.what()};
                    if (message != "TopUpWithDB: writing cache items failed" &&
                        message != "TopUpWithDB: writing descriptor failed") {
                        throw;
                    }
                }
                assert(spk_manager->GetWalletDescriptor().next_index == next_index);
            },
            [&] {
                if (descriptor_writes_blocked || descriptor_metadata_writes_blocked) return;
                LOCK(spk_manager->cs_desc_man);
                const auto descriptor_before{spk_manager->GetWalletDescriptor()};
                if (!descriptor_before.descriptor->IsRange()) return;
                const size_t scripts_before{spk_manager->GetScriptPubKeys().size()};
                const unsigned int keypool_size_before{spk_manager->GetKeyPoolSize()};
                if (keypool_size_before == std::numeric_limits<unsigned int>::max()) return;
                if (!BlockDescriptorMetadataWrites(wallet)) {
                    good_data = false;
                    return;
                }
                descriptor_metadata_writes_blocked = true;

                bool topup_succeeded{false};
                try {
                    topup_succeeded = spk_manager->TopUp(keypool_size_before + 1);
                } catch (const std::runtime_error& error) {
                    if (std::string_view{error.what()} != "TopUpWithDB: writing descriptor failed") {
                        throw;
                    }
                }
                assert(!topup_succeeded);
                assert(spk_manager->GetWalletDescriptor().range_end == descriptor_before.range_end);
                assert(spk_manager->GetKeyPoolSize() == keypool_size_before);
                assert(spk_manager->GetScriptPubKeys().size() == scripts_before);
            },
            [&] {
                if (descriptor_writes_blocked || descriptor_metadata_writes_blocked) return;
                LOCK(spk_manager->cs_desc_man);
                const auto descriptor_before{spk_manager->GetWalletDescriptor()};
                if (!descriptor_before.descriptor->IsRange() ||
                    descriptor_before.range_end == std::numeric_limits<int32_t>::max()) return;
                const size_t scripts_before{spk_manager->GetScriptPubKeys().size()};
                const int32_t end_range_before{spk_manager->GetEndRange()};
                if (!BlockDescriptorWrites(wallet)) {
                    good_data = false;
                    return;
                }
                descriptor_writes_blocked = true;

                WalletDescriptor replacement{descriptor_before};
                replacement.range_end++;
                replacement.cache = {};
                bool update_succeeded{false};
                try {
                    update_succeeded = static_cast<bool>(
                        spk_manager->UpdateWalletDescriptor(replacement, FlatSigningProvider{}));
                } catch (const std::runtime_error& error) {
                    const std::string_view message{error.what()};
                    if (message != "TopUpWithDB: writing cache items failed" &&
                        message != "TopUpWithDB: writing descriptor failed") {
                        throw;
                    }
                }
                assert(!update_succeeded);
                assert(spk_manager->GetScriptPubKeys().size() == scripts_before);
                assert(spk_manager->GetEndRange() == end_range_before);
                assert(spk_manager->GetWalletDescriptor().range_end == descriptor_before.range_end);
            },
            [&] {
                CMutableTransaction tx_to;
                const std::optional<CMutableTransaction> opt_tx_to{ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS)};
                if (!opt_tx_to) {
                    good_data = false;
                    return;
                }
                tx_to = *opt_tx_to;

                std::map<COutPoint, Coin> coins{ConsumeCoins(fuzzed_data_provider)};
                const int sighash{fuzzed_data_provider.ConsumeIntegral<int>()};
                std::map<int, bilingual_str> input_errors;
                (void)spk_manager->SignTransaction(tx_to, coins, sighash, input_errors);
            },
            [&] {
                std::optional<PartiallySignedTransaction> opt_psbt{ConsumeDeserializableConstructor<PartiallySignedTransaction>(fuzzed_data_provider)};
                if (!opt_psbt) {
                    good_data = false;
                    return;
                }
                auto psbt{*opt_psbt};
                std::optional<PrecomputedTransactionData> txdata_res = PrecomputePSBTData(psbt);
                if (!txdata_res) {
                    return;
                }
                const PrecomputedTransactionData& txdata = *txdata_res;
                common::PSBTFillOptions options{
                    .sign = fuzzed_data_provider.ConsumeBool(),
                    .sighash_type = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 151),
                    .finalize = fuzzed_data_provider.ConsumeBool(),
                    .bip32_derivs = fuzzed_data_provider.ConsumeBool()
                };
                if (options.sighash_type == 151) options.sighash_type = std::nullopt;
                (void)spk_manager->FillPSBT(psbt, txdata, options);
            }
        );
        if (descriptor_writes_blocked || descriptor_metadata_writes_blocked) break;
    }

    std::string descriptor;
    (void)spk_manager->GetDescriptorString(descriptor, /*priv=*/fuzzed_data_provider.ConsumeBool());
    (void)spk_manager->GetEndRange();
    (void)spk_manager->GetKeyPoolSize();
}

FUZZ_TARGET(spkm_migration, .init = initialize_spkm_migration)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};
    const auto& node{g_setup->m_node};
    Chainstate& chainstate{node.chainman->ActiveChainstate()};

    std::unique_ptr<CWallet> wallet_ptr{std::make_unique<CWallet>(node.chain.get(), "", CreateMockableWalletDatabase())};
    CWallet& wallet{*wallet_ptr};
    wallet.m_keypool_size = 1;
    {
        LOCK(wallet.cs_wallet);
        wallet.UnsetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        wallet.SetLastBlockProcessed(chainstate.m_chain.Height(), chainstate.m_chain.Tip()->GetBlockHash());
    }

    auto& legacy_data{*wallet.GetOrCreateLegacyDataSPKM()};

    std::vector<CKey> keys;
    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 30) {
        const auto key{ConsumePrivateKey(fuzzed_data_provider)};
        if (!key.IsValid()) return;
        auto pub_key{key.GetPubKey()};
        if (!pub_key.IsFullyValid()) return;
        if (legacy_data.LoadKey(key, pub_key) && std::find(keys.begin(), keys.end(), key) == keys.end()) keys.push_back(key);
    }

    size_t added_chains = 0;
    bool add_hd_chain{fuzzed_data_provider.ConsumeBool() && !keys.empty()};
    CHDChain hd_chain;
    auto version{fuzzed_data_provider.ConsumeBool() ? CHDChain::VERSION_HD_CHAIN_SPLIT : CHDChain::VERSION_HD_BASE};
    CKey hd_key;
    if (add_hd_chain) {
        hd_key = PickValue(fuzzed_data_provider, keys);
        hd_chain.nVersion = version;
        hd_chain.seed_id = hd_key.GetPubKey().GetID();
        legacy_data.LoadHDChain(hd_chain);
        added_chains++;
    }

    bool add_inactive_hd_chain{fuzzed_data_provider.ConsumeBool() && !keys.empty()};
    if (add_inactive_hd_chain) {
        CKey inactive_hd_key = PickValue(fuzzed_data_provider, keys);
        hd_chain.nVersion = fuzzed_data_provider.ConsumeBool() ? CHDChain::VERSION_HD_CHAIN_SPLIT : CHDChain::VERSION_HD_BASE;
        bool dup_chain = hd_key.IsValid() && std::equal(hd_key.begin(), hd_key.end(), inactive_hd_key.begin());
        hd_chain.seed_id = inactive_hd_key.GetPubKey().GetID();
        legacy_data.AddInactiveHDChain(hd_chain);
        if (!dup_chain) added_chains++;
    }

    bool watch_only = false;
    const auto pub_key = ConsumeDeserializable<CPubKey>(fuzzed_data_provider);
    if (!pub_key || !pub_key->IsFullyValid()) return;
    auto script_dest{GetScriptForDestination(WitnessV0KeyHash{*pub_key})};
    if (fuzzed_data_provider.ConsumeBool()) {
        script_dest = GetScriptForDestination(CTxDestination{PKHash(*pub_key)});
    }
    if (legacy_data.LoadWatchOnly(script_dest)) watch_only = true;

    size_t added_script{0};
    bool good_data{true};
    LIMITED_WHILE (good_data && fuzzed_data_provider.ConsumeBool(), 30) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                CKey key;
                if (!keys.empty()) {
                    key = PickValue(fuzzed_data_provider, keys);
                } else {
                    key = ConsumePrivateKey(fuzzed_data_provider, /*compressed=*/fuzzed_data_provider.ConsumeBool());
                }
                if (!key.IsValid()) return;
                auto pub_key{key.GetPubKey()};
                CScript script;
                CallOneOf(
                    fuzzed_data_provider,
                    [&] {
                        script = GetScriptForDestination(CTxDestination{PKHash(pub_key)});
                    },
                    [&] {
                        script = GetScriptForDestination(WitnessV0KeyHash(pub_key));
                    },
                    [&] {
                        std::optional<CScript> script_opt{ConsumeDeserializable<CScript>(fuzzed_data_provider)};
                        if (!script_opt) {
                            good_data = false;
                            return;
                        }
                        script = script_opt.value();
                    }
                );
                if (fuzzed_data_provider.ConsumeBool()) script = GetScriptForDestination(ScriptHash(script));
                if (!legacy_data.HaveCScript(CScriptID(script)) && legacy_data.AddCScript(script)) added_script++;
            },
            [&] {
                CKey key;
                if (!keys.empty()) {
                    key = PickValue(fuzzed_data_provider, keys);
                } else {
                    key = ConsumePrivateKey(fuzzed_data_provider, /*compressed=*/fuzzed_data_provider.ConsumeBool());
                }
                if (!key.IsValid()) return;
                const auto num_keys{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, MAX_PUBKEYS_PER_MULTISIG)};
                std::vector<CPubKey> pubkeys;
                pubkeys.emplace_back(key.GetPubKey());
                for (size_t i = 1; i < num_keys; i++) {
                    if (fuzzed_data_provider.ConsumeBool()) {
                        pubkeys.emplace_back(key.GetPubKey());
                    } else {
                        CKey private_key{ConsumePrivateKey(fuzzed_data_provider, /*compressed=*/fuzzed_data_provider.ConsumeBool())};
                        if (!private_key.IsValid()) return;
                        pubkeys.emplace_back(private_key.GetPubKey());
                    }
                }
                if (pubkeys.size() < num_keys) return;
                CScript multisig_script{GetScriptForMultisig(num_keys, pubkeys)};
                if (!legacy_data.HaveCScript(CScriptID(multisig_script)) && legacy_data.AddCScript(multisig_script)) {
                    added_script++;
                }
            }
        );
    }

    auto result{legacy_data.MigrateToDescriptor()};
    assert(result);
    if ((add_hd_chain && version >= CHDChain::VERSION_HD_CHAIN_SPLIT) || (!add_hd_chain && add_inactive_hd_chain)) {
        added_chains *= 2;
    }
    size_t added_size{keys.size() + added_chains};
    if (added_script > 0) {
        assert(result->desc_spkms.size() >= added_size);
    } else {
        assert(result->desc_spkms.size() == added_size);
    }
    if (watch_only) assert(!result->watch_descs.empty());
    if (!result->solvable_descs.empty()) assert(added_script > 0);
}

} // namespace
} // namespace wallet
