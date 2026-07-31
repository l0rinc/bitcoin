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
    //! #71 c5: per-block verification-progress schedule (monotonic,
    //! flat, or adversarial non-monotonic per fuzz mode).
    std::vector<double> progress;

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
    double guessVerificationProgress(const uint256& hash) override
    {
        if (progress.empty()) return 1.0;
        const int height{IndexOf(hash)};
        return height >= 0 ? progress[height] : 1.0;
    }
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
    // #71 c5: fuzz the verification-progress schedule EARLY (control-flag
    // position — late consumption starves on exhausted providers). Modes:
    // monotonic non-decreasing (the real-chain contract), flat (hits the
    // divide guard), adversarial non-monotonic (out-of-contract interface).
    const int prog_mode{fdp.ConsumeIntegralInRange<int>(0, 2)};
    {
        const size_t total{static_cast<size_t>(n_blocks + n_ext)};
        double p{fdp.ConsumeProbability<double>() * 0.5};
        for (size_t i = 0; i < total; ++i) {
            if (prog_mode == 0) {
                p = std::min(1.0, p + fdp.ConsumeProbability<double>() / (total + 1));
            } else if (prog_mode == 1) {
                // flat: keep p
            } else {
                p = fdp.ConsumeProbability<double>();
            }
            chain.progress.push_back(p);
        }
    }
    const bool have_mempool_tx{fdp.ConsumeBool()};
    const int64_t now_step{fdp.ConsumeBool() ? 61 : 1};

    // Build the fuzz-driven chain.
    std::vector<CTransactionRef> wallet_txs(n_blocks);
    int64_t max_time{0};
    for (int i = 0; i < n_blocks; ++i) {
        FuzzRescanChain::Block b;
        b.hash = ConsumeUInt256(fdp);
        if (b.hash.IsNull()) b.hash = uint256::ONE; // keep last_failed_block distinguishable from unset
        // Keep block hashes unique (a real chain can never repeat one): bump
        // the low byte until it differs from every earlier block (#71 c3 —
        // the previous null->ONE correction could produce duplicate hashes).
        while (std::any_of(chain.blocks.begin(), chain.blocks.end(), [&](const auto& prev) { return prev.hash == b.hash; })) {
            *reinterpret_cast<uint8_t*>(b.hash.data()) += 1;
        }
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
        // Keep extension hashes unique against the original chain and each
        // other — same rule as the original blocks (#71 c3/c4: a duplicate
        // here let a hash-replaced start "resume" at a same-hash extension
        // block, an unrepresentable chain).
        while (std::any_of(chain.blocks.begin(), chain.blocks.end(), [&](const auto& prev) { return prev.hash == eb.hash; }) ||
               std::any_of(chain.extension.begin(), chain.extension.end(), [&](const auto& prev) { return prev.hash == eb.hash; })) {
            *reinterpret_cast<uint8_t*>(eb.hash.data()) += 1;
        }
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
        // #71 c5: with a monotonic or flat progress schedule the wallet's
        // reported scanning progress stays within [0, 1]; a flat schedule
        // (zero denominator) yields exactly 0 via the divide guard.
        if (!chain.progress.empty() && prog_mode <= 1) {
            const double sp{wallet->ScanningProgress()};
            Assert(sp >= 0.0 && sp <= 1.0);
            if (prog_mode == 1) Assert(sp == 0.0);
        }
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
        // the grown tip (the wallet's last block advanced mid-scan) —
        // unless the flip deactivated an original block BEFORE the scan
        // reached it (#71 c4: the flip fires on the first findBlock from
        // ANY caller, including pre-scan start-path callers; a deactivated
        // successor stops the scan legitimately one block early via the
        // wallet.cpp "previous block no longer on the chain" break).
        if (chain.extend_triggered && !use_max_height) {
            const int expected{chain.flip_triggered ? chain.flip_idx - 1 : n_blocks + n_ext - 1};
            Assert(scanned_upto == expected);
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

    // ---- Crash-resume durability invariant (#71 c2) ----
    // A save_progress scan persists its position (BESTBLOCK record). A
    // later process resuming from that record must not miss any wallet
    // transaction in the remaining range, and resuming must not lose
    // previously found transactions.
    if (save_progress && result.last_scanned_height.has_value()) {
        const uint256 recorded{WITH_LOCK(wallet->cs_wallet, return wallet->GetLastBlockHash())};
        int recorded_idx{-1};
        for (size_t i{0}; i < chain.blocks.size(); ++i) {
            if (chain.blocks[i].hash == recorded) { recorded_idx = static_cast<int>(i); break; }
        }
        // v1 scope: resume only when the recorded position is an original
        // chain block we can locate deterministically (extension-block
        // resume is a separate cell).
        if (recorded_idx >= 0 && recorded_idx + 1 < n_blocks) {
            const size_t wallet_size_before{WITH_LOCK(wallet->cs_wallet, return wallet->mapWallet.size())};
            int64_t now_calls2{0};
            reserver.setNow([&]() { return SteadyClock::time_point{SteadyClock::duration{2000 + (++now_calls2) * now_step}}; });
            const auto resume_result{wallet->ScanForWalletTransactions(recorded, recorded_idx, std::nullopt, reserver, /*save_progress=*/false)};
            // The resume must not have lost any previously found wallet txs.
            const size_t wallet_size_after{WITH_LOCK(wallet->cs_wallet, return wallet->mapWallet.size())};
            Assert(wallet_size_after >= wallet_size_before);
            // And every wallet tx in blocks after the recorded position that
            // the original scan saw must be present after the resume.
            if (resume_result.status == CWallet::ScanResult::SUCCESS) {
                for (int i = recorded_idx + 1; i < n_blocks; ++i) {
                    if (!wallet_txs[i] || !chain.blocks[i].active || !chain.blocks[i].has_data) continue;
                    LOCK(wallet->cs_wallet);
                    Assert(wallet->mapWallet.contains(wallet_txs[i]->GetHash()));
                }
            }
        }
        // ---- Extension-block resume (#71 c4) ----
        // When the recorded position lies inside the triggered extension
        // (the mock moved extension blocks into `blocks`), resuming from it
        // must cover the remaining extension blocks: no tx loss, no FAILURE
        // (extension blocks are always active and readable), and on SUCCESS
        // every later extension wallet tx is present.
        // ---- Extension-block resume (#71 c4) ----
        // The natural schedule never leaves the recorded position inside
        // the extension (abort is requested pre-scan, extension blocks are
        // always active/readable, and the last save lands on the stop/tip),
        // so force the class deterministically: when the extension
        // triggered with >= 2 blocks, resume from the FIRST extension block
        // and require SUCCESS plus every later extension wallet tx (they
        // are all active and readable, and no abort/shutdown is pending
        // inside this block).
        if (chain.extend_triggered && n_ext >= 2) {
            const uint256& ext_start{chain.blocks[n_blocks].hash};
            const size_t wallet_size_before{WITH_LOCK(wallet->cs_wallet, return wallet->mapWallet.size())};
            int64_t now_calls4{0};
            reserver.setNow([&]() { return SteadyClock::time_point{SteadyClock::duration{2000 + (++now_calls4) * now_step}}; });
            const auto ext_resume{wallet->ScanForWalletTransactions(ext_start, n_blocks, std::nullopt, reserver, /*save_progress=*/false)};
            const size_t wallet_size_after{WITH_LOCK(wallet->cs_wallet, return wallet->mapWallet.size())};
            Assert(wallet_size_after >= wallet_size_before);
            Assert(ext_resume.status == CWallet::ScanResult::SUCCESS);
            for (int i = n_blocks + 1; i < static_cast<int>(chain.blocks.size()); ++i) {
                const CTransactionRef& tx{ext_wallet_txs[i - n_blocks]};
                if (!tx) continue;
                LOCK(wallet->cs_wallet);
                Assert(wallet->mapWallet.contains(tx->GetHash()));
            }
        }
        // ---- Reorged recorded position (#71 c3) ----
        // The mock's flip can only deactivate mid-scan, so the recorded
        // position is always active here. Force the post-scan reorg classes
        // deterministically: (a) deactivate the recorded block in place,
        // (b) replace it with a fresh hash (second-generation chain view).
        // The resume must fail cleanly (FAILURE with last_failed_block ==
        // recorded), never scan wrong-chain data, and lose nothing.
        if (recorded_idx >= 0) {
            const uint256 fresh{chain.blocks[recorded_idx].hash};
            for (int mode = 0; mode < 2; ++mode) {
                if (mode == 0) {
                    chain.blocks[recorded_idx].active = false;
                } else {
                    // second-generation hash, guaranteed unknown to the mock
                    uint256 replaced{fresh};
                    do {
                        *reinterpret_cast<uint8_t*>(replaced.data()) += 0xa5;
                    } while (std::any_of(chain.blocks.begin(), chain.blocks.end(),
                                         [&](const auto& prev) { return prev.hash == replaced; }));
                    chain.blocks[recorded_idx].hash = replaced;
                }
                const size_t wallet_size_before{WITH_LOCK(wallet->cs_wallet, return wallet->mapWallet.size())};
                int64_t now_calls3{0};
                reserver.setNow([&]() { return SteadyClock::time_point{SteadyClock::duration{2000 + (++now_calls3) * now_step}}; });
                const auto reorg_result{wallet->ScanForWalletTransactions(
                    recorded, recorded_idx, std::nullopt, reserver, /*save_progress=*/false)};
                // A pending abort/shutdown yields USER_ABORT with nothing
                // scanned (legitimate); otherwise the resume must FAIL with
                // last_failed_block == recorded. SUCCESS (scanning over a
                // reorged start) is the defect shape and must never occur.
                Assert(reorg_result.status != CWallet::ScanResult::SUCCESS);
                if (reorg_result.status == CWallet::ScanResult::FAILURE) {
                    Assert(reorg_result.last_failed_block == recorded);
                }
                const size_t wallet_size_after{WITH_LOCK(wallet->cs_wallet, return wallet->mapWallet.size())};
                Assert(wallet_size_after >= wallet_size_before);
                chain.blocks[recorded_idx].active = true;
                chain.blocks[recorded_idx].hash = fresh;
            }
        }
    }
}

} // namespace
} // namespace wallet
