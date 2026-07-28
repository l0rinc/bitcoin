// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <chain.h>
#include <interfaces/chain.h>
#include <key_io.h>
#include <outputtype.h>
#include <policy/rbf.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/descriptor.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/time.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace wallet {
namespace {

using interfaces::FoundBlock;
using interfaces::Handler;
using interfaces::SettingsAction;
using interfaces::SettingsUpdate;

//! Minimal in-memory interfaces::Chain whose block list, per-block
//! active/on-disk-data flags, one mid-scan deactivation ("reorg"), and
//! shutdown state are all driven by the fuzz input. Lets
//! ScanForWalletTransactions reach states that are racy or impossible to
//! drive deterministically against a real chainstate: a block that goes
//! inactive between the nextBlock check and its own iteration
//! (block_still_active == false), missing block data (read failure),
//! and shutdown interruption.
class FuzzRescanChain final : public interfaces::Chain
{
public:
    struct Block {
        uint256 hash;
        int64_t time;
        int64_t max_time;
        bool active;
        bool has_data;
        CBlock data;
    };

    std::vector<Block> blocks;
    bool shutdown{false};
    std::optional<CTransactionRef> mempool_tx;
    //! When >= 0, blocks[flip_idx] is deactivated the first time
    //! findBlock() is called for its hash (mid-scan reorg).
    int flip_idx{-1};
    bool flip_triggered{false};
    //! When >= 0, on the first findBlock() for extend_at, the chain grows
    //! by extension and the wallet's last processed block advances to the
    //! new tip (deterministic mid-scan block-connection).
    CWallet* wallet{nullptr};
    int extend_at{-1};
    std::vector<Block> extension;
    bool extend_triggered{false};

private:
    const Block* Find(const uint256& hash) const
    {
        for (size_t i = 0; i < blocks.size(); ++i) {
            if (blocks[i].hash == hash) return &blocks[i];
        }
        return nullptr;
    }
    int IndexOf(const uint256& hash) const
    {
        for (size_t i = 0; i < blocks.size(); ++i) {
            if (blocks[i].hash == hash) return static_cast<int>(i);
        }
        return -1;
    }

public:
    std::optional<int> getHeight() override
    {
        if (blocks.empty()) return std::nullopt;
        return static_cast<int>(blocks.size()) - 1;
    }
    uint256 getBlockHash(int height) override { return blocks.at(height).hash; }
    bool haveBlockOnDisk(int height) override { return blocks.at(height).has_data; }
    std::optional<int> findLocatorFork(const CBlockLocator&) override { return std::nullopt; }
    bool hasBlockFilterIndex(BlockFilterType) override { return false; }
    std::optional<bool> blockFilterMatchesAny(BlockFilterType, const uint256&, const GCSFilter::ElementSet&) override { return std::nullopt; }

    bool findBlock(const uint256& hash, const FoundBlock& fb) override
    {
        const int height{IndexOf(hash)};
        if (height < 0) return false;
        if (height == flip_idx && !flip_triggered) {
            blocks[flip_idx].active = false;
            flip_triggered = true;
        }
        if (height == extend_at && !extend_triggered) {
            extend_triggered = true;
            int64_t ext_max{blocks.empty() ? 0 : blocks.back().max_time};
            for (Block& eb : extension) {
                ext_max = std::max(ext_max, eb.time);
                eb.max_time = ext_max;
                blocks.push_back(std::move(eb));
            }
            if (!extension.empty()) {
                LOCK2(::cs_main, wallet->cs_wallet);
                wallet->SetLastBlockProcessed(static_cast<int>(blocks.size()) - 1, blocks.back().hash);
            }
            extension.clear();
        }
        const Block& b{blocks[height]};
        fb.found = true;
        if (fb.m_hash) *fb.m_hash = b.hash;
        if (fb.m_height) *fb.m_height = height;
        if (fb.m_time) *fb.m_time = b.time;
        if (fb.m_max_time) *fb.m_max_time = b.max_time;
        if (fb.m_mtp_time) *fb.m_mtp_time = b.time;
        if (fb.m_in_active_chain) *fb.m_in_active_chain = b.active;
        if (fb.m_locator && b.active) *fb.m_locator = CBlockLocator{{b.hash}};
        if (fb.m_next_block) {
            const FoundBlock* nb{fb.m_next_block};
            if (b.active && height + 1 < static_cast<int>(blocks.size()) && blocks[height + 1].active) {
                nb->found = true;
                if (nb->m_hash) *nb->m_hash = blocks[height + 1].hash;
                if (nb->m_in_active_chain) *nb->m_in_active_chain = true;
            } else {
                nb->found = false;
            }
        }
        if (fb.m_data) {
            if (b.has_data) {
                *fb.m_data = b.data;
            } else {
                fb.m_data->SetNull();
            }
        }
        return true;
    }

    bool findFirstBlockWithTimeAndHeight(int64_t min_time, int min_height, const FoundBlock& fb) override
    {
        for (size_t i = 0; i < blocks.size(); ++i) {
            if (static_cast<int>(i) >= min_height && blocks[i].time >= min_time && blocks[i].active) {
                return findBlock(blocks[i].hash, fb);
            }
        }
        return false;
    }
    bool findAncestorByHeight(const uint256& hash, int ancestor_height, const FoundBlock& out) override
    {
        const int height{IndexOf(hash)};
        if (height < 0 || ancestor_height < 0 || ancestor_height > height) return false;
        return findBlock(blocks[ancestor_height].hash, out);
    }
    bool findAncestorByHash(const uint256& block_hash, const uint256& ancestor_hash, const FoundBlock& ancestor_out) override
    {
        const int height{IndexOf(block_hash)};
        const int ancestor_height{IndexOf(ancestor_hash)};
        if (height < 0 || ancestor_height < 0 || ancestor_height > height) return false;
        return findBlock(blocks[ancestor_height].hash, ancestor_out);
    }
    bool findCommonAncestor(const uint256& block_hash1, const uint256& block_hash2, const FoundBlock& ancestor_out, const FoundBlock& block1_out, const FoundBlock& block2_out) override
    {
        const int h1{IndexOf(block_hash1)};
        const int h2{IndexOf(block_hash2)};
        if (h1 < 0 || h2 < 0) return false;
        const int ancestor{std::min(h1, h2)};
        findBlock(blocks[h1].hash, block1_out);
        findBlock(blocks[h2].hash, block2_out);
        return findBlock(blocks[ancestor].hash, ancestor_out);
    }
    bool broadcastTransaction(const CTransactionRef&, const CAmount&, node::TxBroadcast, std::string&) override { return true; }
    void findCoins(std::map<COutPoint, Coin>&) override {}
    double guessVerificationProgress(const uint256&) override { return 1.0; }
    bool hasBlocks(const uint256& block_hash, int min_height, std::optional<int> max_height) override
    {
        const int height{IndexOf(block_hash)};
        if (height < 0) return false;
        const int top{max_height ? std::min(*max_height, height) : height};
        for (int i = min_height; i <= top; ++i) {
            if (!blocks[i].has_data) return false;
        }
        return true;
    }
    RBFTransactionState isRBFOptIn(const CTransaction&) override { return RBFTransactionState::UNKNOWN; }
    bool isInMempool(const Txid&) override { return false; }
    bool hasDescendantsInMempool(const Txid&) override { return false; }
    void getTransactionAncestry(const Txid&, size_t& ancestors, size_t& cluster_count, size_t*, CAmount*) override
    {
        ancestors = 1;
        cluster_count = 1;
    }
    std::map<COutPoint, CAmount> calculateIndividualBumpFees(const std::vector<COutPoint>&, const CFeeRate&) override { return {}; }
    std::optional<CAmount> calculateCombinedBumpFee(const std::vector<COutPoint>&, const CFeeRate&) override { return std::nullopt; }
    void getPackageLimits(unsigned int& limit_ancestor_count, unsigned int& limit_descendant_count) override
    {
        limit_ancestor_count = 25;
        limit_descendant_count = 25;
    }
    util::Result<void> checkChainLimits(const CTransactionRef&) override { return {}; }
    CFeeRate estimateSmartFee(int, bool, FeeCalculation*) override { return CFeeRate{1000}; }
    unsigned int estimateMaxBlocks() override { return 1; }
    CFeeRate mempoolMinFee() override { return CFeeRate{1000}; }
    CFeeRate relayMinFee() override { return CFeeRate{1000}; }
    CFeeRate relayIncrementalFee() override { return CFeeRate{1000}; }
    CFeeRate relayDustFee() override { return CFeeRate{1000}; }
    bool havePruned() override { return false; }
    std::optional<int> getPruneHeight() override { return std::nullopt; }
    bool isReadyToBroadcast() override { return true; }
    bool isInitialBlockDownload() override { return false; }
    bool shutdownRequested() override { return shutdown; }
    void initMessage(const std::string&) override {}
    void initWarning(const bilingual_str&) override {}
    void initError(const bilingual_str&) override {}
    void showProgress(const std::string&, int, bool) override {}
    std::unique_ptr<Handler> handleNotifications(std::shared_ptr<Notifications>) override { return nullptr; }
    void waitForNotificationsIfTipChanged(const uint256&) override {}
    void waitForNotifications() override {}
    std::unique_ptr<Handler> handleRpc(const CRPCCommand&) override { return nullptr; }
    bool rpcEnableDeprecated(const std::string&) override { return false; }
    common::SettingsValue getSetting(const std::string&) override { return {}; }
    std::vector<common::SettingsValue> getSettingsList(const std::string&) override { return {}; }
    common::SettingsValue getRwSetting(const std::string&) override { return {}; }
    bool updateRwSetting(const std::string&, const SettingsUpdate&) override { return false; }
    bool overwriteRwSetting(const std::string&, common::SettingsValue, SettingsAction) override { return false; }
    bool deleteRwSettings(const std::string&, SettingsAction) override { return false; }
    void requestMempoolTransactions(Notifications& notifications) override
    {
        if (mempool_tx) notifications.transactionAddedToMempool(*mempool_tx);
    }
    bool hasAssumedValidChain() override { return false; }
};

TestingSetup* g_setup;

void initialize_rescan()
{
    static const auto testing_setup = MakeNoLogFileContext<TestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(wallet_rescan, .init = initialize_rescan)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};
    (void)g_setup;
    SetMockTime(fdp.ConsumeIntegralInRange<int64_t>(946684800, 4133980799)); // wallet construction uses GetTime

    FuzzRescanChain chain;
    auto wallet{std::make_unique<CWallet>(&chain, "", CreateMockableWalletDatabase())};
    wallet->m_keypool_size = 1; // avoid ~1000 BIP32 derivations per iteration (scriptpubkeyman.cpp precedent)

    // Give the wallet a spendable descriptor and a receive script.
    wallet->SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
    wallet->SetupDescriptorScriptPubKeyMans();
    CKey key;
    key.MakeNewKey(/*fCompressed=*/true);
    FlatSigningProvider provider;
    std::string error;
    auto descs{Parse("combo(" + EncodeSecret(key) + ")", provider, error, /*require_checksum=*/false)};
    if (descs.size() != 1) return;
    WalletDescriptor w_desc{std::move(descs.at(0)), 0, 0, 1, 1};
    if (!wallet->AddWalletDescriptor(w_desc, provider, "", false)) return;
    const auto dest{wallet->GetNewDestination(OutputType::BECH32, "")};
    if (!dest) return;
    const CScript wallet_script{GetScriptForDestination(*dest)};

    // Control flags FIRST: short inputs would otherwise exhaust the buffer
    // in the block loop and never reach them (2000/2000 all-SUCCESS run).
    const int n_blocks{fdp.ConsumeIntegralInRange<int>(1, 24)};
    chain.shutdown = fdp.ConsumeBool() && fdp.ConsumeBool();               // ~1/4
    const bool abort{fdp.ConsumeBool() && fdp.ConsumeBool()};              // ~1/4
    const bool save_progress{fdp.ConsumeBool()};
    const bool use_max_height{fdp.ConsumeBool()};
    const int max_height{use_max_height ? fdp.ConsumeIntegralInRange<int>(0, n_blocks - 1) : 0};
    if (fdp.ConsumeBool() && fdp.ConsumeBool()) {                          // ~1/4 schedule a mid-scan reorg
        chain.flip_idx = fdp.ConsumeIntegralInRange<int>(0, n_blocks - 1);
    }
    if (fdp.ConsumeBool() && fdp.ConsumeBool()) {                          // ~1/4 schedule mid-scan tip extension
        chain.extend_at = fdp.ConsumeIntegralInRange<int>(0, n_blocks - 1);
    }
    const int n_ext{chain.extend_at >= 0 ? fdp.ConsumeIntegralInRange<int>(1, 6) : 0};
    const bool have_mempool_tx{fdp.ConsumeBool()};
    const int64_t now_step{fdp.ConsumeBool() ? 61 : 1};

    // Build the fuzz-driven chain.
    std::vector<CTransactionRef> wallet_txs(n_blocks);
    int64_t max_time{0};
    for (int i = 0; i < n_blocks; ++i) {
        FuzzRescanChain::Block b;
        b.hash = ConsumeUInt256(fdp);
        if (b.hash.IsNull()) b.hash = uint256::ONE; // keep last_failed_block distinguishable from unset
        b.time = fdp.ConsumeIntegralInRange<int64_t>(1, 1LL << 40);
        max_time = std::max(max_time, b.time);
        b.max_time = max_time;
        b.active = !(fdp.ConsumeBool() && fdp.ConsumeBool());              // ~3/4 active
        b.has_data = !(fdp.ConsumeBool() && fdp.ConsumeBool() && fdp.ConsumeBool()); // ~7/8 present
        CMutableTransaction cb;
        cb.vin.emplace_back(COutPoint{Txid::FromUint256(ConsumeUInt256(fdp)), 0});
        cb.vout.emplace_back(CAmount{1}, CScript{} << OP_1);
        b.data.vtx.push_back(MakeTransactionRef(std::move(cb)));
        b.data.nBits = 0x207fffff; // CBlock::IsNull() checks nBits == 0; the scan treats that as a failed read
        if (fdp.ConsumeBool()) {                                           // ~1/2 contain a wallet tx
            CMutableTransaction mtx;
            mtx.vin.emplace_back(COutPoint{Txid::FromUint256(ConsumeUInt256(fdp)), 1});
            mtx.vout.emplace_back(CAmount{1000 + i}, wallet_script);
            wallet_txs[i] = MakeTransactionRef(std::move(mtx));
            b.data.vtx.push_back(wallet_txs[i]);
        }
        chain.blocks.push_back(std::move(b));
    }

    // Pre-build the extension blocks (active, with data, maybe wallet txs).
    std::vector<CTransactionRef> ext_wallet_txs(n_ext);
    for (int i = 0; i < n_ext; ++i) {
        FuzzRescanChain::Block eb;
        eb.hash = ConsumeUInt256(fdp);
        if (eb.hash.IsNull()) eb.hash = uint256::ONE;
        eb.time = fdp.ConsumeIntegralInRange<int64_t>(1, 1LL << 40);
        eb.active = true;
        eb.has_data = true;
        CMutableTransaction cb;
        cb.vin.emplace_back(COutPoint{Txid::FromUint256(ConsumeUInt256(fdp)), 0});
        cb.vout.emplace_back(CAmount{1}, CScript{} << OP_1);
        eb.data.vtx.push_back(MakeTransactionRef(std::move(cb)));
        eb.data.nBits = 0x207fffff;
        if (fdp.ConsumeBool()) {
            CMutableTransaction mtx;
            mtx.vin.emplace_back(COutPoint{Txid::FromUint256(ConsumeUInt256(fdp)), 1});
            mtx.vout.emplace_back(CAmount{2000 + i}, wallet_script);
            ext_wallet_txs[i] = MakeTransactionRef(std::move(mtx));
            eb.data.vtx.push_back(ext_wallet_txs[i]);
        }
        chain.extension.push_back(std::move(eb));
    }
    chain.wallet = wallet.get();

    if (have_mempool_tx) {
        CMutableTransaction mtx;
        mtx.vin.emplace_back(COutPoint{Txid::FromUint256(ConsumeUInt256(fdp)), 2});
        mtx.vout.emplace_back(CAmount{7}, wallet_script);
        chain.mempool_tx = MakeTransactionRef(std::move(mtx));
    }

    // The wallet sees the mock tip as its last processed block so the scan
    // walks the whole mock chain.
    {
        LOCK2(::cs_main, wallet->cs_wallet);
        wallet->SetLastBlockProcessed(n_blocks - 1, chain.blocks.back().hash);
    }
    WalletRescanReserver reserver{*wallet};
    if (!reserver.reserve()) return;
    // reserve() discards any pre-existing abort request by design, so the
    // abort must be requested after it to take effect on this scan.
    if (abort) wallet->AbortRescan();
    // Deterministic time, optionally past the 60s progress interval so the
    // "Still rescanning" and save-progress branches are reachable.
    int64_t now_calls{0};
    reserver.setNow([&]() { return SteadyClock::time_point{SteadyClock::duration{1000 + (++now_calls) * now_step}}; });

    const auto result{wallet->ScanForWalletTransactions(chain.blocks.front().hash, 0,
                                                        use_max_height ? std::optional<int>{max_height} : std::nullopt,
                                                        reserver, save_progress)};

    // ---- Crash-independent oracles ----
    const bool flip_at_zero{chain.flip_idx == 0 && chain.flip_triggered};
    bool data_gap_scanned{false};
    const int scanned_upto{result.last_scanned_height ? *result.last_scanned_height : -1};
    for (int i = 0; i <= scanned_upto; ++i) {
        if (!chain.blocks[i].has_data) data_gap_scanned = true;
    }

    if (result.status == CWallet::ScanResult::SUCCESS) {
        Assert(!abort && !chain.shutdown);
        // Every scanned block was active and readable; a deactivation may
        // only happen at blocks the scan never reached (or right at the
        // stop point's successor, which the scan then never processes).
        Assert(!flip_at_zero);
        Assert(!(chain.flip_triggered && chain.flip_idx <= scanned_upto));
        for (int i = 0; i <= scanned_upto; ++i) {
            Assert(chain.blocks[i].active && chain.blocks[i].has_data);
        }
        Assert(result.last_failed_block.IsNull());
        // A triggered extension without max_height must extend the scan to
        // the grown tip (the wallet's last block advanced mid-scan).
        if (chain.extend_triggered && !use_max_height) {
            Assert(scanned_upto == n_blocks + n_ext - 1);
        }
    } else if (result.status == CWallet::ScanResult::FAILURE) {
        // A failure must have a reason: unreadable scanned block, an
        // initially-inactive start block, or the triggered deactivation.
        Assert(data_gap_scanned || !chain.blocks.front().active || chain.flip_triggered);
        Assert(!result.last_failed_block.IsNull());
    } else {
        Assert(result.status == CWallet::ScanResult::USER_ABORT);
        Assert(abort || chain.shutdown);
    }
    if (result.last_scanned_height) {
        const FuzzRescanChain::Block& last{chain.blocks[*result.last_scanned_height]};
        Assert(last.active && last.has_data);
    }

    // Wallet transactions in fully scanned blocks must be in the wallet.
    for (int i = 0; i <= scanned_upto; ++i) {
        if (i < n_blocks && (!wallet_txs[i] || !chain.blocks[i].active || !chain.blocks[i].has_data)) continue;
        if (i < n_blocks) {
            LOCK(wallet->cs_wallet);
            Assert(wallet->mapWallet.contains(wallet_txs[i]->GetHash()));
        }
    }
    // Extension-block wallet transactions likewise, once scanned past the
    // original tip.
    for (int i = n_blocks; i <= scanned_upto; ++i) {
        const CTransactionRef& tx{ext_wallet_txs[i - n_blocks]};
        if (!tx) continue;
        LOCK(wallet->cs_wallet);
        Assert(wallet->mapWallet.contains(tx->GetHash()));
    }
}

} // namespace
} // namespace wallet
