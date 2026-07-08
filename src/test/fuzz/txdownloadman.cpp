// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <node/context.h>
#include <node/mempool_args.h>
#include <node/miner.h>
#include <node/txdownloadman.h>
#include <node/txdownloadman_impl.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/mempool.h>
#include <test/util/mining.h>
#include <test/util/script.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/hasher.h>
#include <util/rbf.h>
#include <util/time.h>
#include <validation.h>
#include <validationinterface.h>

#include <algorithm>
#include <array>
#include <set>

namespace {

const TestingSetup* g_setup;

constexpr size_t NUM_COINS{50};
COutPoint COINS[NUM_COINS];

static TxValidationResult TESTED_TX_RESULTS[] = {
    // Skip TX_RESULT_UNSET
    TxValidationResult::TX_CONSENSUS,
    TxValidationResult::TX_INPUTS_NOT_STANDARD,
    TxValidationResult::TX_NOT_STANDARD,
    TxValidationResult::TX_MISSING_INPUTS,
    TxValidationResult::TX_PREMATURE_SPEND,
    TxValidationResult::TX_WITNESS_MUTATED,
    TxValidationResult::TX_WITNESS_STRIPPED,
    TxValidationResult::TX_CONFLICT,
    TxValidationResult::TX_MEMPOOL_POLICY,
    // Skip TX_NO_MEMPOOL
    TxValidationResult::TX_RECONSIDERABLE,
    TxValidationResult::TX_UNKNOWN,
};

// Precomputed transactions. Some may conflict with each other.
std::vector<CTransactionRef> TRANSACTIONS;

// Limit the total number of peers because we don't expect coverage to change much with lots more peers.
constexpr int NUM_PEERS = 16;

// Precomputed random durations (positive and negative, each ~exponentially distributed).
std::chrono::microseconds TIME_SKIPS[128];

static CTransactionRef MakeTransactionSpending(const std::vector<COutPoint>& outpoints, size_t num_outputs, bool add_witness)
{
    CMutableTransaction tx;
    // If no outpoints are given, create a random one.
    for (const auto& outpoint : outpoints) {
        tx.vin.emplace_back(outpoint);
    }
    if (add_witness) {
        tx.vin[0].scriptWitness.stack.push_back({1});
    }
    for (size_t o = 0; o < num_outputs; ++o) tx.vout.emplace_back(CENT, P2WSH_OP_TRUE);
    return MakeTransactionRef(tx);
}
static std::vector<COutPoint> PickCoins(FuzzedDataProvider& fuzzed_data_provider)
{
    std::vector<COutPoint> ret;
    ret.push_back(fuzzed_data_provider.PickValueInArray(COINS));
    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10) {
        ret.push_back(fuzzed_data_provider.PickValueInArray(COINS));
    }
    return ret;
}

void initialize()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
    for (uint32_t i = 0; i < uint32_t{NUM_COINS}; ++i) {
        COINS[i] = COutPoint{Txid::FromUint256((HashWriter() << i).GetHash()), i};
    }
    size_t outpoints_index = 0;
    // 2 transactions same txid different witness
    {
        auto tx1{MakeTransactionSpending({COINS[outpoints_index]}, /*num_outputs=*/5, /*add_witness=*/false)};
        auto tx2{MakeTransactionSpending({COINS[outpoints_index]}, /*num_outputs=*/5, /*add_witness=*/true)};
        Assert(tx1->GetHash() == tx2->GetHash());
        TRANSACTIONS.emplace_back(tx1);
        TRANSACTIONS.emplace_back(tx2);
        outpoints_index += 1;
    }
    // 2 parents 1 child
    {
        auto tx_parent_1{MakeTransactionSpending({COINS[outpoints_index++]}, /*num_outputs=*/1, /*add_witness=*/true)};
        TRANSACTIONS.emplace_back(tx_parent_1);
        auto tx_parent_2{MakeTransactionSpending({COINS[outpoints_index++]}, /*num_outputs=*/1, /*add_witness=*/false)};
        TRANSACTIONS.emplace_back(tx_parent_2);
        TRANSACTIONS.emplace_back(MakeTransactionSpending({COutPoint{tx_parent_1->GetHash(), 0}, COutPoint{tx_parent_2->GetHash(), 0}},
                                                            /*num_outputs=*/1, /*add_witness=*/true));
    }
    // 1 parent 2 children
    {
        auto tx_parent{MakeTransactionSpending({COINS[outpoints_index++]}, /*num_outputs=*/2, /*add_witness=*/true)};
        TRANSACTIONS.emplace_back(tx_parent);
        TRANSACTIONS.emplace_back(MakeTransactionSpending({COutPoint{tx_parent->GetHash(), 0}},
                                                            /*num_outputs=*/1, /*add_witness=*/true));
        TRANSACTIONS.emplace_back(MakeTransactionSpending({COutPoint{tx_parent->GetHash(), 1}},
                                                            /*num_outputs=*/1, /*add_witness=*/true));
    }
    // chain of 5 segwit
    {
        COutPoint& last_outpoint = COINS[outpoints_index++];
        for (auto i{0}; i < 5; ++i) {
            auto tx{MakeTransactionSpending({last_outpoint}, /*num_outputs=*/1, /*add_witness=*/true)};
            TRANSACTIONS.emplace_back(tx);
            last_outpoint = COutPoint{tx->GetHash(), 0};
        }
    }
    // chain of 5 non-segwit
    {
        COutPoint& last_outpoint = COINS[outpoints_index++];
        for (auto i{0}; i < 5; ++i) {
            auto tx{MakeTransactionSpending({last_outpoint}, /*num_outputs=*/1, /*add_witness=*/false)};
            TRANSACTIONS.emplace_back(tx);
            last_outpoint = COutPoint{tx->GetHash(), 0};
        }
    }
    // Also create a loose tx for each outpoint. Some of these transactions conflict with the above
    // or have the same txid.
    for (const auto& outpoint : COINS) {
        TRANSACTIONS.emplace_back(MakeTransactionSpending({outpoint}, /*num_outputs=*/1, /*add_witness=*/true));
    }

    // Create random-looking time jumps
    int i = 0;
    // TIME_SKIPS[N] for N=0..15 is just N microseconds.
    for (; i < 16; ++i) {
        TIME_SKIPS[i] = std::chrono::microseconds{i};
    }
    // TIME_SKIPS[N] for N=16..127 has randomly-looking but roughly exponentially increasing values up to
    // 198.416453 seconds.
    for (; i < 128; ++i) {
        int diff_bits = ((i - 10) * 2) / 9;
        uint64_t diff = 1 + (CSipHasher(0, 0).Write(i).Finalize() >> (64 - diff_bits));
        TIME_SKIPS[i] = TIME_SKIPS[i - 1] + std::chrono::microseconds{diff};
    }
}

void CheckPackageToValidate(const node::PackageToValidate& package_to_validate, NodeId peer)
{
    Assert(package_to_validate.m_senders.size() == 2);
    Assert(package_to_validate.m_senders.front() == peer);
    Assert(package_to_validate.m_senders.back() < NUM_PEERS);

    // Package is a 1p1c
    const auto& package = package_to_validate.m_txns;
    Assert(std::all_of(package.cbegin(), package.cend(), [](const auto& tx) { return tx != nullptr; }));
    Assert(IsChildWithParents(package));
    Assert(package.size() == 2);
}

void CheckUniqueParents(const std::vector<Txid>& unique_parents, const CTransaction& tx)
{
    Assert(std::is_sorted(unique_parents.begin(), unique_parents.end()));
    Assert(std::adjacent_find(unique_parents.begin(), unique_parents.end()) == unique_parents.end());
    Assert(unique_parents.size() <= tx.vin.size());
}

void AssertNoTxRequest(const node::TxDownloadManagerImpl& txdownload_impl, const uint256& txhash)
{
    std::vector<NodeId> peers;
    txdownload_impl.m_txrequest.GetCandidatePeers(txhash, peers);
    Assert(peers.empty());
}

void AssertNoTxRequestFromPeer(const node::TxDownloadManagerImpl& txdownload_impl, const uint256& txhash, NodeId peer)
{
    std::vector<NodeId> peers;
    txdownload_impl.m_txrequest.GetCandidatePeers(txhash, peers);
    Assert(std::find(peers.begin(), peers.end(), peer) == peers.end());
}

void AssertNoTxRequestsForTx(const node::TxDownloadManagerImpl& txdownload_impl, const CTransactionRef& tx)
{
    AssertNoTxRequest(txdownload_impl, tx->GetHash().ToUint256());
    AssertNoTxRequest(txdownload_impl, tx->GetWitnessHash().ToUint256());
}

void AssertNoTxRequestsFromPeerForTx(const node::TxDownloadManagerImpl& txdownload_impl, const CTransactionRef& tx, NodeId peer)
{
    AssertNoTxRequestFromPeer(txdownload_impl, tx->GetHash().ToUint256(), peer);
    AssertNoTxRequestFromPeer(txdownload_impl, tx->GetWitnessHash().ToUint256(), peer);
}

FUZZ_TARGET(txdownloadman, .init = initialize)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};

    // Initialize txdownloadman
    bilingual_str error;
    CTxMemPool pool{MemPoolOptionsForTest(g_setup->m_node), error};
    FastRandomContext det_rand{true};
    node::TxDownloadManager txdownloadman{node::TxDownloadOptions{pool, det_rand, true}};

    std::chrono::microseconds time{244466666};

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 500) {
        NodeId rand_peer = fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(0, NUM_PEERS - 1);

        // Transaction can be one of the premade ones or a randomly generated one
        auto rand_tx = fuzzed_data_provider.ConsumeBool() ?
            MakeTransactionSpending(PickCoins(fuzzed_data_provider),
                                    /*num_outputs=*/fuzzed_data_provider.ConsumeIntegralInRange(1, 500),
                                    /*add_witness=*/fuzzed_data_provider.ConsumeBool()) :
            TRANSACTIONS.at(fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(0, TRANSACTIONS.size() - 1));

        CallOneOf(
            fuzzed_data_provider,
            [&] {
                node::TxDownloadConnectionInfo info{
                    .m_preferred = fuzzed_data_provider.ConsumeBool(),
                    .m_relay_permissions = fuzzed_data_provider.ConsumeBool(),
                    .m_wtxid_relay = fuzzed_data_provider.ConsumeBool()
                };
                txdownloadman.ConnectedPeer(rand_peer, info);
            },
            [&] {
                txdownloadman.DisconnectedPeer(rand_peer);
                txdownloadman.CheckIsEmpty(rand_peer);
            },
            [&] {
                txdownloadman.ActiveTipChange();
            },
            [&] {
                CBlock block;
                block.vtx.push_back(rand_tx);
                txdownloadman.BlockConnected(std::make_shared<CBlock>(block));
            },
            [&] {
                txdownloadman.BlockDisconnected();
            },
            [&] {
                txdownloadman.MempoolAcceptedTx(rand_tx);
            },
            [&] {
                TxValidationState state;
                state.Invalid(fuzzed_data_provider.PickValueInArray(TESTED_TX_RESULTS), "");
                bool first_time_failure{fuzzed_data_provider.ConsumeBool()};

                node::RejectedTxTodo todo = txdownloadman.MempoolRejectedTx(rand_tx, state, rand_peer, first_time_failure);
                Assert(first_time_failure || !todo.m_should_add_extra_compact_tx);
                CheckUniqueParents(todo.m_unique_parents, *rand_tx);
            },
            [&] {
                auto gtxid = fuzzed_data_provider.ConsumeBool() ?
                             GenTxid{rand_tx->GetHash()} :
                             GenTxid{rand_tx->GetWitnessHash()};
                txdownloadman.AddTxAnnouncement(rand_peer, gtxid, time);
            },
            [&] {
                txdownloadman.GetRequestsToSend(rand_peer, time);
                Assert(txdownloadman.GetRequestsToSend(rand_peer, time).empty());
            },
            [&] {
                const auto& [should_validate, maybe_package] = txdownloadman.ReceivedTx(rand_peer, rand_tx);
                // The only possible results should be:
                // - Don't validate the tx, no package.
                // - Don't validate the tx, package.
                // - Validate the tx, no package.
                // The only combination that doesn't make sense is validate both tx and package.
                Assert(!(should_validate && maybe_package.has_value()));
                if (maybe_package.has_value()) {
                    CheckPackageToValidate(*maybe_package, rand_peer);
                    const auto package_hash{GetPackageHash(maybe_package->m_txns)};
                    if (fuzzed_data_provider.ConsumeBool()) {
                        txdownloadman.MempoolRejectedPackage(maybe_package->m_txns);
                        const auto& [validate_after_reject, next_package] = txdownloadman.ReceivedTx(rand_peer, maybe_package->m_txns.front());
                        Assert(!validate_after_reject);
                        if (next_package.has_value()) {
                            CheckPackageToValidate(*next_package, rand_peer);
                            Assert(GetPackageHash(next_package->m_txns) != package_hash);
                        }
                    }
                }
            },
            [&] {
                txdownloadman.ReceivedNotFound(rand_peer, {rand_tx->GetWitnessHash()});
            },
            [&] {
                const bool expect_work{txdownloadman.HaveMoreWork(rand_peer)};
                const auto ptx = txdownloadman.GetTxToReconsider(rand_peer);
                // expect_work=true doesn't necessarily mean the next item from the workset isn't a
                // nullptr, as the transaction could have been removed from orphanage without being
                // removed from the peer's workset.
                if (ptx) {
                    // However, if there was a non-null tx in the workset, HaveMoreWork should have
                    // returned true.
                    Assert(expect_work);
                }
            });
        // Jump forwards or backwards
        auto time_skip = fuzzed_data_provider.PickValueInArray(TIME_SKIPS);
        if (fuzzed_data_provider.ConsumeBool()) time_skip *= -1;
        time += time_skip;
    }
    // Disconnect everybody, check that all data structures are empty.
    for (NodeId nodeid = 0; nodeid < NUM_PEERS; ++nodeid) {
        txdownloadman.DisconnectedPeer(nodeid);
        txdownloadman.CheckIsEmpty(nodeid);
    }
    txdownloadman.CheckIsEmpty();
}

// Give node 0 relay permissions, and nobody else. This helps us remember who is a RelayPermissions
// peer without tracking anything (this is only for the txdownload_impl target).
static bool HasRelayPermissions(NodeId peer) { return peer == 0; }

static void CheckOrphanagePublicView(const node::TxDownloadManagerImpl& txdownload_impl)
{
    const auto orphan_infos{txdownload_impl.m_orphanage->GetOrphanTransactions()};
    Assert(orphan_infos.size() == txdownload_impl.m_orphanage->CountUniqueOrphans());

    std::set<Wtxid> seen_wtxids;
    node::TxOrphanage::Count announcements{0};
    node::TxOrphanage::Count latency_score{0};
    node::TxOrphanage::Usage total_usage{0};
    std::array<node::TxOrphanage::Count, NUM_PEERS> announcements_by_peer{};
    std::array<node::TxOrphanage::Count, NUM_PEERS> latency_by_peer{};
    std::array<node::TxOrphanage::Usage, NUM_PEERS> usage_by_peer{};

    for (const auto& orphan_info : orphan_infos) {
        Assert(orphan_info.tx);
        const auto& tx{*orphan_info.tx};
        const auto wtxid{tx.GetWitnessHash()};
        Assert(seen_wtxids.insert(wtxid).second);
        Assert(txdownload_impl.m_orphanage->HaveTx(wtxid));
        const auto tx_from_orphanage{txdownload_impl.m_orphanage->GetTx(wtxid)};
        Assert(tx_from_orphanage);
        Assert(tx_from_orphanage->GetHash() == tx.GetHash());
        Assert(tx_from_orphanage->GetWitnessHash() == wtxid);
        Assert(!orphan_info.announcers.empty());

        const auto tx_weight{GetTransactionWeight(tx)};
        const auto tx_latency{static_cast<node::TxOrphanage::Count>(1 + tx.vin.size() / 10)};
        total_usage += tx_weight;
        latency_score += tx_latency - 1;
        announcements += static_cast<node::TxOrphanage::Count>(orphan_info.announcers.size());

        for (const auto peer : orphan_info.announcers) {
            Assert(peer >= 0 && peer < NUM_PEERS);
            Assert(txdownload_impl.m_orphanage->HaveTxFromPeer(wtxid, peer));
            Assert(txdownload_impl.m_peer_info.contains(peer));
            const auto peer_index{static_cast<size_t>(peer)};
            announcements_by_peer.at(peer_index) += 1;
            latency_by_peer.at(peer_index) += tx_latency;
            usage_by_peer.at(peer_index) += tx_weight;
        }
    }

    latency_score += announcements;
    Assert(announcements == txdownload_impl.m_orphanage->CountAnnouncements());
    Assert(total_usage == txdownload_impl.m_orphanage->TotalOrphanUsage());
    Assert(latency_score == txdownload_impl.m_orphanage->TotalLatencyScore());
    Assert(total_usage <= txdownload_impl.m_orphanage->MaxGlobalUsage());
    Assert(latency_score <= txdownload_impl.m_orphanage->MaxGlobalLatencyScore());

    for (NodeId peer = 0; peer < NUM_PEERS; ++peer) {
        const auto peer_index{static_cast<size_t>(peer)};
        Assert(announcements_by_peer.at(peer_index) == txdownload_impl.m_orphanage->AnnouncementsFromPeer(peer));
        Assert(latency_by_peer.at(peer_index) == txdownload_impl.m_orphanage->LatencyScoreFromPeer(peer));
        Assert(usage_by_peer.at(peer_index) == txdownload_impl.m_orphanage->UsageByPeer(peer));
    }
}

static void CheckInvariants(const node::TxDownloadManagerImpl& txdownload_impl)
{
    txdownload_impl.m_orphanage->SanityCheck();
    const auto wtxid_peers{std::count_if(txdownload_impl.m_peer_info.begin(), txdownload_impl.m_peer_info.end(),
                                         [](const auto& peer) {
                                             return peer.second.m_connection_info.m_wtxid_relay;
                                         })};
    Assert(txdownload_impl.m_num_wtxid_peers == static_cast<uint32_t>(wtxid_peers));
    // We should never have more than the maximum in-flight requests out for a peer.
    for (NodeId peer = 0; peer < NUM_PEERS; ++peer) {
        if (!HasRelayPermissions(peer)) {
            Assert(txdownload_impl.m_txrequest.Count(peer) <= node::MAX_PEER_TX_ANNOUNCEMENTS);
        }
        if (!txdownload_impl.m_peer_info.contains(peer)) {
            Assert(txdownload_impl.m_txrequest.Count(peer) == 0);
            Assert(txdownload_impl.m_orphanage->UsageByPeer(peer) == 0);
        }
    }
    txdownload_impl.m_txrequest.SanityCheck();
}

FUZZ_TARGET(txdownloadman_impl, .init = initialize)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};

    // Initialize a TxDownloadManagerImpl
    bilingual_str error;
    CTxMemPool pool{MemPoolOptionsForTest(g_setup->m_node), error};
    FastRandomContext det_rand{true};
    node::TxDownloadManagerImpl txdownload_impl{node::TxDownloadOptions{pool, det_rand, true}};

    std::chrono::microseconds time{244466666};

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 500) {
        NodeId rand_peer = fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(0, NUM_PEERS - 1);

        // Transaction can be one of the premade ones or a randomly generated one
        auto rand_tx = fuzzed_data_provider.ConsumeBool() ?
            MakeTransactionSpending(PickCoins(fuzzed_data_provider),
                                    /*num_outputs=*/fuzzed_data_provider.ConsumeIntegralInRange(1, 500),
                                    /*add_witness=*/fuzzed_data_provider.ConsumeBool()) :
            TRANSACTIONS.at(fuzzed_data_provider.ConsumeIntegralInRange<unsigned>(0, TRANSACTIONS.size() - 1));

        CallOneOf(
            fuzzed_data_provider,
            [&] {
                node::TxDownloadConnectionInfo info{
                    .m_preferred = fuzzed_data_provider.ConsumeBool(),
                    .m_relay_permissions = HasRelayPermissions(rand_peer),
                    .m_wtxid_relay = fuzzed_data_provider.ConsumeBool()
                };
                const bool was_connected{txdownload_impl.m_peer_info.contains(rand_peer)};
                bool preferred_before{false};
                bool relay_permissions_before{false};
                bool wtxid_relay_before{false};
                if (was_connected) {
                    const auto& existing_info{txdownload_impl.m_peer_info.at(rand_peer).m_connection_info};
                    preferred_before = existing_info.m_preferred;
                    relay_permissions_before = existing_info.m_relay_permissions;
                    wtxid_relay_before = existing_info.m_wtxid_relay;
                }
                const auto peer_count_before{txdownload_impl.m_peer_info.size()};
                const auto wtxid_peers_before{txdownload_impl.m_num_wtxid_peers};
                const auto txrequest_size_before{txdownload_impl.m_txrequest.Size()};
                const auto txrequest_count_before{txdownload_impl.m_txrequest.Count(rand_peer)};
                const auto txrequest_candidates_before{txdownload_impl.m_txrequest.CountCandidates(rand_peer)};
                const auto txrequest_inflight_before{txdownload_impl.m_txrequest.CountInFlight(rand_peer)};
                const auto orphan_count_before{txdownload_impl.m_orphanage->CountUniqueOrphans()};
                const auto orphan_announcements_total_before{txdownload_impl.m_orphanage->CountAnnouncements()};
                const auto orphan_usage_total_before{txdownload_impl.m_orphanage->TotalOrphanUsage()};
                const auto orphan_usage_before{txdownload_impl.m_orphanage->UsageByPeer(rand_peer)};
                const auto orphan_announcements_before{txdownload_impl.m_orphanage->AnnouncementsFromPeer(rand_peer)};
                const auto orphan_work_before{txdownload_impl.m_orphanage->HaveTxToReconsider(rand_peer)};
                txdownload_impl.ConnectedPeer(rand_peer, info);
                Assert(txdownload_impl.m_txrequest.Size() == txrequest_size_before);
                Assert(txdownload_impl.m_txrequest.Count(rand_peer) == txrequest_count_before);
                Assert(txdownload_impl.m_txrequest.CountCandidates(rand_peer) == txrequest_candidates_before);
                Assert(txdownload_impl.m_txrequest.CountInFlight(rand_peer) == txrequest_inflight_before);
                Assert(txdownload_impl.m_orphanage->CountUniqueOrphans() == orphan_count_before);
                Assert(txdownload_impl.m_orphanage->CountAnnouncements() == orphan_announcements_total_before);
                Assert(txdownload_impl.m_orphanage->TotalOrphanUsage() == orphan_usage_total_before);
                Assert(txdownload_impl.m_orphanage->UsageByPeer(rand_peer) == orphan_usage_before);
                Assert(txdownload_impl.m_orphanage->AnnouncementsFromPeer(rand_peer) == orphan_announcements_before);
                Assert(txdownload_impl.m_orphanage->HaveTxToReconsider(rand_peer) == orphan_work_before);
                const auto it{txdownload_impl.m_peer_info.find(rand_peer)};
                Assert(it != txdownload_impl.m_peer_info.end());
                if (was_connected) {
                    Assert(txdownload_impl.m_peer_info.size() == peer_count_before);
                    Assert(txdownload_impl.m_num_wtxid_peers == wtxid_peers_before);
                    Assert(it->second.m_connection_info.m_preferred == preferred_before);
                    Assert(it->second.m_connection_info.m_relay_permissions == relay_permissions_before);
                    Assert(it->second.m_connection_info.m_wtxid_relay == wtxid_relay_before);
                } else {
                    Assert(txdownload_impl.m_peer_info.size() == peer_count_before + 1);
                    Assert(txdownload_impl.m_num_wtxid_peers == wtxid_peers_before + (info.m_wtxid_relay ? 1 : 0));
                    Assert(it->second.m_connection_info.m_preferred == info.m_preferred);
                    Assert(it->second.m_connection_info.m_relay_permissions == info.m_relay_permissions);
                    Assert(it->second.m_connection_info.m_wtxid_relay == info.m_wtxid_relay);
                }
            },
            [&] {
                txdownload_impl.DisconnectedPeer(rand_peer);
                txdownload_impl.CheckIsEmpty(rand_peer);
            },
            [&] {
                struct PeerInfoSnapshot {
                    bool present{false};
                    bool preferred{false};
                    bool relay_permissions{false};
                    bool wtxid_relay{false};
                };
                std::array<PeerInfoSnapshot, NUM_PEERS> peer_info_before;
                std::array<size_t, NUM_PEERS> txrequest_count_before;
                std::array<size_t, NUM_PEERS> txrequest_candidates_before;
                std::array<size_t, NUM_PEERS> txrequest_inflight_before;
                std::array<node::TxOrphanage::Usage, NUM_PEERS> orphan_usage_before;
                std::array<node::TxOrphanage::Count, NUM_PEERS> orphan_announcements_before;
                std::array<bool, NUM_PEERS> orphan_work_before;
                for (NodeId peer = 0; peer < NUM_PEERS; ++peer) {
                    if (const auto it{txdownload_impl.m_peer_info.find(peer)}; it != txdownload_impl.m_peer_info.end()) {
                        peer_info_before[peer] = {
                            .present = true,
                            .preferred = it->second.m_connection_info.m_preferred,
                            .relay_permissions = it->second.m_connection_info.m_relay_permissions,
                            .wtxid_relay = it->second.m_connection_info.m_wtxid_relay,
                        };
                    }
                    txrequest_count_before[peer] = txdownload_impl.m_txrequest.Count(peer);
                    txrequest_candidates_before[peer] = txdownload_impl.m_txrequest.CountCandidates(peer);
                    txrequest_inflight_before[peer] = txdownload_impl.m_txrequest.CountInFlight(peer);
                    orphan_usage_before[peer] = txdownload_impl.m_orphanage->UsageByPeer(peer);
                    orphan_announcements_before[peer] = txdownload_impl.m_orphanage->AnnouncementsFromPeer(peer);
                    orphan_work_before[peer] = txdownload_impl.m_orphanage->HaveTxToReconsider(peer);
                }
                const auto txrequest_size_before{txdownload_impl.m_txrequest.Size()};
                const auto orphan_count_before{txdownload_impl.m_orphanage->CountUniqueOrphans()};
                const auto orphan_announcements_total_before{txdownload_impl.m_orphanage->CountAnnouncements()};
                const auto orphan_usage_total_before{txdownload_impl.m_orphanage->TotalOrphanUsage()};
                const auto wtxid_peers_before{txdownload_impl.m_num_wtxid_peers};
                txdownload_impl.ActiveTipChange();
                Assert(txdownload_impl.m_num_wtxid_peers == wtxid_peers_before);
                Assert(txdownload_impl.m_txrequest.Size() == txrequest_size_before);
                Assert(txdownload_impl.m_orphanage->CountUniqueOrphans() == orphan_count_before);
                Assert(txdownload_impl.m_orphanage->CountAnnouncements() == orphan_announcements_total_before);
                Assert(txdownload_impl.m_orphanage->TotalOrphanUsage() == orphan_usage_total_before);
                for (NodeId peer = 0; peer < NUM_PEERS; ++peer) {
                    const auto it{txdownload_impl.m_peer_info.find(peer)};
                    Assert((it != txdownload_impl.m_peer_info.end()) == peer_info_before[peer].present);
                    if (it != txdownload_impl.m_peer_info.end()) {
                        Assert(it->second.m_connection_info.m_preferred == peer_info_before[peer].preferred);
                        Assert(it->second.m_connection_info.m_relay_permissions == peer_info_before[peer].relay_permissions);
                        Assert(it->second.m_connection_info.m_wtxid_relay == peer_info_before[peer].wtxid_relay);
                    }
                    Assert(txdownload_impl.m_txrequest.Count(peer) == txrequest_count_before[peer]);
                    Assert(txdownload_impl.m_txrequest.CountCandidates(peer) == txrequest_candidates_before[peer]);
                    Assert(txdownload_impl.m_txrequest.CountInFlight(peer) == txrequest_inflight_before[peer]);
                    Assert(txdownload_impl.m_orphanage->UsageByPeer(peer) == orphan_usage_before[peer]);
                    Assert(txdownload_impl.m_orphanage->AnnouncementsFromPeer(peer) == orphan_announcements_before[peer]);
                    Assert(txdownload_impl.m_orphanage->HaveTxToReconsider(peer) == orphan_work_before[peer]);
                }
                // After a block update, nothing should be in the rejection caches
                for (const auto& tx : TRANSACTIONS) {
                    Assert(!txdownload_impl.RecentRejectsFilter().contains(tx->GetWitnessHash().ToUint256()));
                    Assert(!txdownload_impl.RecentRejectsFilter().contains(tx->GetHash().ToUint256()));
                    Assert(!txdownload_impl.RecentRejectsReconsiderableFilter().contains(tx->GetWitnessHash().ToUint256()));
                    Assert(!txdownload_impl.RecentRejectsReconsiderableFilter().contains(tx->GetHash().ToUint256()));
                }
            },
            [&] {
                CBlock block;
                block.vtx.push_back(rand_tx);
                txdownload_impl.BlockConnected(std::make_shared<CBlock>(block));
                // Block transactions must be removed from orphanage
                Assert(!txdownload_impl.m_orphanage->HaveTx(rand_tx->GetWitnessHash()));
                AssertNoTxRequestsForTx(txdownload_impl, rand_tx);
                Assert(txdownload_impl.RecentConfirmedTransactionsFilter().contains(rand_tx->GetHash().ToUint256()));
                if (rand_tx->HasWitness()) {
                    Assert(txdownload_impl.RecentConfirmedTransactionsFilter().contains(rand_tx->GetWitnessHash().ToUint256()));
                }
            },
            [&] {
                txdownload_impl.BlockDisconnected();
                Assert(!txdownload_impl.RecentConfirmedTransactionsFilter().contains(rand_tx->GetWitnessHash().ToUint256()));
                Assert(!txdownload_impl.RecentConfirmedTransactionsFilter().contains(rand_tx->GetHash().ToUint256()));
            },
            [&] {
                txdownload_impl.MempoolAcceptedTx(rand_tx);
                Assert(!txdownload_impl.m_orphanage->HaveTx(rand_tx->GetWitnessHash()));
                AssertNoTxRequestsForTx(txdownload_impl, rand_tx);
            },
            [&] {
                TxValidationState state;
                state.Invalid(fuzzed_data_provider.PickValueInArray(TESTED_TX_RESULTS), "");
                bool first_time_failure{fuzzed_data_provider.ConsumeBool()};

                node::RejectedTxTodo todo = txdownload_impl.MempoolRejectedTx(rand_tx, state, rand_peer, first_time_failure);
                Assert(first_time_failure || !todo.m_should_add_extra_compact_tx);
                CheckUniqueParents(todo.m_unique_parents, *rand_tx);
                if (state.GetResult() != TxValidationResult::TX_MISSING_INPUTS) {
                    Assert(!txdownload_impl.m_orphanage->HaveTx(rand_tx->GetWitnessHash()));
                }
            },
            [&] {
                auto gtxid = fuzzed_data_provider.ConsumeBool() ?
                             GenTxid{rand_tx->GetHash()} :
                             GenTxid{rand_tx->GetWitnessHash()};
                txdownload_impl.AddTxAnnouncement(rand_peer, gtxid, time);
            },
            [&] {
                const auto getdata_requests = txdownload_impl.GetRequestsToSend(rand_peer, time);
                // TxDownloadManager should not be telling us to request things we already have.
                // Exclude m_lazy_recent_rejects_reconsiderable because it may request low-feerate parent of orphan.
                std::vector<uint256> requested_hashes;
                for (const auto& gtxid : getdata_requests) {
                    Assert(!txdownload_impl.AlreadyHaveTx(gtxid, /*include_reconsiderable=*/false));
                    const auto txhash{gtxid.ToUint256()};
                    Assert(std::find(requested_hashes.begin(), requested_hashes.end(), txhash) == requested_hashes.end());
                    requested_hashes.push_back(txhash);

                    std::vector<NodeId> peers;
                    txdownload_impl.m_txrequest.GetCandidatePeers(txhash, peers);
                    Assert(std::find(peers.begin(), peers.end(), rand_peer) != peers.end());
                }
                Assert(txdownload_impl.m_txrequest.CountInFlight(rand_peer) >= getdata_requests.size());
                Assert(txdownload_impl.GetRequestsToSend(rand_peer, time).empty());
            },
            [&] {
                const bool known_peer{txdownload_impl.m_peer_info.contains(rand_peer)};
                const auto& [should_validate, maybe_package] = txdownload_impl.ReceivedTx(rand_peer, rand_tx);
                AssertNoTxRequestsFromPeerForTx(txdownload_impl, rand_tx, rand_peer);
                if (!known_peer) {
                    Assert(!should_validate);
                    Assert(!maybe_package.has_value());
                }
                // The only possible results should be:
                // - Don't validate the tx, no package.
                // - Don't validate the tx, package.
                // - Validate the tx, no package.
                // The only combination that doesn't make sense is validate both tx and package.
                Assert(!(should_validate && maybe_package.has_value()));
                if (should_validate) {
                    Assert(!txdownload_impl.AlreadyHaveTx(rand_tx->GetWitnessHash(), /*include_reconsiderable=*/true));
                }
                if (maybe_package.has_value()) {
                    CheckPackageToValidate(*maybe_package, rand_peer);

                    const auto& package = maybe_package->m_txns;
                    // Parent is in m_lazy_recent_rejects_reconsiderable and child is in m_orphanage
                    Assert(txdownload_impl.RecentRejectsReconsiderableFilter().contains(rand_tx->GetWitnessHash().ToUint256()));
                    Assert(txdownload_impl.m_orphanage->HaveTx(maybe_package->m_txns.back()->GetWitnessHash()));
                    // Package has not been rejected
                    const auto package_hash{GetPackageHash(package)};
                    Assert(!txdownload_impl.RecentRejectsReconsiderableFilter().contains(package_hash));
                    // Neither is in m_lazy_recent_rejects
                    Assert(!txdownload_impl.RecentRejectsFilter().contains(package.front()->GetWitnessHash().ToUint256()));
                    Assert(!txdownload_impl.RecentRejectsFilter().contains(package.back()->GetWitnessHash().ToUint256()));
                    if (fuzzed_data_provider.ConsumeBool()) {
                        txdownload_impl.MempoolRejectedPackage(package);
                        Assert(txdownload_impl.RecentRejectsReconsiderableFilter().contains(package_hash));
                        const auto& [validate_after_reject, next_package] = txdownload_impl.ReceivedTx(rand_peer, package.front());
                        Assert(!validate_after_reject);
                        if (next_package.has_value()) {
                            CheckPackageToValidate(*next_package, rand_peer);
                            Assert(GetPackageHash(next_package->m_txns) != package_hash);
                        }
                    }
                }
            },
            [&] {
                txdownload_impl.ReceivedNotFound(rand_peer, {rand_tx->GetWitnessHash()});
                AssertNoTxRequestFromPeer(txdownload_impl, rand_tx->GetWitnessHash().ToUint256(), rand_peer);
            },
            [&] {
                const bool expect_work{txdownload_impl.HaveMoreWork(rand_peer)};
                const auto ptx{txdownload_impl.GetTxToReconsider(rand_peer)};
                // expect_work=true doesn't necessarily mean the next item from the workset isn't a
                // nullptr, as the transaction could have been removed from orphanage without being
                // removed from the peer's workset.
                if (ptx) {
                    // However, if there was a non-null tx in the workset, HaveMoreWork should have
                    // returned true.
                    Assert(expect_work);
                    Assert(txdownload_impl.AlreadyHaveTx(ptx->GetWitnessHash(), /*include_reconsiderable=*/false));
                    // Presumably we have validated this tx. Use "missing inputs" to keep it in the
                    // orphanage longer. Later iterations might call MempoolAcceptedTx or
                    // MempoolRejectedTx with a different error.
                    TxValidationState state_missing_inputs;
                    state_missing_inputs.Invalid(TxValidationResult::TX_MISSING_INPUTS, "");
                    txdownload_impl.MempoolRejectedTx(ptx, state_missing_inputs, rand_peer, fuzzed_data_provider.ConsumeBool());
                }
            });

        CheckInvariants(txdownload_impl);
        if (txdownload_impl.m_orphanage->CountAnnouncements() <= 64) {
            CheckOrphanagePublicView(txdownload_impl);
        }
        auto time_skip = fuzzed_data_provider.PickValueInArray(TIME_SKIPS);
        if (fuzzed_data_provider.ConsumeBool()) time_skip *= -1;
        time += time_skip;
    }
    CheckInvariants(txdownload_impl);
    CheckOrphanagePublicView(txdownload_impl);
    // Disconnect everybody, check that all data structures are empty.
    for (NodeId nodeid = 0; nodeid < NUM_PEERS; ++nodeid) {
        txdownload_impl.DisconnectedPeer(nodeid);
        txdownload_impl.CheckIsEmpty(nodeid);
    }
    txdownload_impl.CheckIsEmpty();
}

} // namespace
