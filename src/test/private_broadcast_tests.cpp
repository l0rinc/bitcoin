// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/transaction.h>
#include <private_broadcast.h>
#include <test/util/setup_common.h>
#include <util/time.h>

#include <algorithm>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(private_broadcast_tests, BasicTestingSetup)

static CTransactionRef MakeDummyTx(uint32_t id, size_t num_witness)
{
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].nSequence = id;
    if (num_witness > 0) {
        mtx.vin[0].scriptWitness = CScriptWitness{};
        mtx.vin[0].scriptWitness.stack.resize(num_witness);
    }
    return MakeTransactionRef(mtx);
}

BOOST_AUTO_TEST_CASE(basic)
{
    SetMockTime(Now<NodeSeconds>());

    PrivateBroadcast pb;
    const NodeId recipient1{1};
    in_addr ipv4Addr;
    ipv4Addr.s_addr = 0xa0b0c001;
    const CService addr1{ipv4Addr, 1111};

    // No transactions initially.
    CHECK(!pb.PickTxForSend(/*will_send_to_nodeid=*/recipient1, /*will_send_to_address=*/addr1).has_value());
    CHECK_EQUAL(pb.GetStale().size(), std::remove_cvref_t<decltype(pb.GetStale().size())>{0});
    CHECK(!pb.HavePendingTransactions());
    CHECK_EQUAL(pb.GetBroadcastInfo().size(), std::remove_cvref_t<decltype(pb.GetBroadcastInfo().size())>{0});

    // Make a transaction and add it.
    const auto tx1{MakeDummyTx(/*id=*/1, /*num_witness=*/0)};

    CHECK(pb.Add(tx1));
    CHECK(!pb.Add(tx1));

    // Make another transaction with same txid, different wtxid and add it.
    const auto tx2{MakeDummyTx(/*id=*/1, /*num_witness=*/1)};
    CHECK(tx1->GetHash() == tx2->GetHash());
    CHECK(tx1->GetWitnessHash() != tx2->GetWitnessHash());

    CHECK(pb.Add(tx2));
    const auto find_tx_info{[](auto& infos, const CTransactionRef& tx) -> const PrivateBroadcast::TxBroadcastInfo& {
        const auto it{std::ranges::find(infos, tx->GetWitnessHash(), [](const auto& info) { return info.tx->GetWitnessHash(); })};
        CHECK(it != infos.end());
        return *it;
    }};
    const auto check_peer_counts{[&](size_t tx1_peer_count, size_t tx2_peer_count) {
        const auto infos{pb.GetBroadcastInfo()};
        CHECK_EQUAL(infos.size(), std::remove_cvref_t<decltype(infos.size())>{2});
        CHECK_EQUAL(find_tx_info(infos, tx1).peers.size(), tx1_peer_count);
        CHECK_EQUAL(find_tx_info(infos, tx2).peers.size(), tx2_peer_count);
    }};

    check_peer_counts(/*tx1_peer_count=*/0, /*tx2_peer_count=*/0);

    const auto tx_for_recipient1{pb.PickTxForSend(/*will_send_to_nodeid=*/recipient1, /*will_send_to_address=*/addr1).value()};
    CHECK(tx_for_recipient1 == tx1 || tx_for_recipient1 == tx2);

    // A second pick must return the other transaction.
    const NodeId recipient2{2};
    const CService addr2{ipv4Addr, 2222};
    const auto tx_for_recipient2{pb.PickTxForSend(/*will_send_to_nodeid=*/recipient2, /*will_send_to_address=*/addr2).value()};
    CHECK(tx_for_recipient2 == tx1 || tx_for_recipient2 == tx2);
    CHECK_NE(tx_for_recipient1, tx_for_recipient2);

    check_peer_counts(/*tx1_peer_count=*/1, /*tx2_peer_count=*/1);

    const NodeId nonexistent_recipient{0};

    // Confirm transactions <-> recipients mapping is correct.
    CHECK(!pb.GetTxForNode(nonexistent_recipient).has_value());
    CHECK_EQUAL(pb.GetTxForNode(recipient1).value(), tx_for_recipient1);
    CHECK_EQUAL(pb.GetTxForNode(recipient2).value(), tx_for_recipient2);

    // Confirm none of the transactions' reception have been confirmed.
    CHECK(!pb.DidNodeConfirmReception(recipient1));
    CHECK(!pb.DidNodeConfirmReception(recipient2));
    CHECK(!pb.DidNodeConfirmReception(nonexistent_recipient));

    // 1. Freshly added transactions should NOT be stale yet.
    CHECK_EQUAL(pb.GetStale().size(), std::remove_cvref_t<decltype(pb.GetStale().size())>{0});

    // 2. Fast-forward the mock clock past the INITIAL_STALE_DURATION.
    SetMockTime(Now<NodeSeconds>() + PrivateBroadcast::INITIAL_STALE_DURATION + 1min);

    // 3. Now that the initial duration has passed, both unconfirmed transactions should be stale.
    CHECK_EQUAL(pb.GetStale().size(), std::remove_cvref_t<decltype(pb.GetStale().size())>{2});

    // Confirm reception by recipient1.
    pb.NodeConfirmedReception(nonexistent_recipient); // Dummy call.
    pb.NodeConfirmedReception(recipient1);

    CHECK(pb.DidNodeConfirmReception(recipient1));
    CHECK(!pb.DidNodeConfirmReception(recipient2));

    const auto infos{pb.GetBroadcastInfo()};
    CHECK_EQUAL(infos.size(), std::remove_cvref_t<decltype(infos.size())>{2});
    {
        const auto& peers{find_tx_info(infos, tx_for_recipient1).peers};
        CHECK_EQUAL(peers.size(), std::remove_cvref_t<decltype(peers.size())>{1});
        CHECK_EQUAL(peers[0].address.ToStringAddrPort(), addr1.ToStringAddrPort());
        CHECK(peers[0].received.has_value());
    }
    {
        const auto& peers{find_tx_info(infos, tx_for_recipient2).peers};
        CHECK_EQUAL(peers.size(), std::remove_cvref_t<decltype(peers.size())>{1});
        CHECK_EQUAL(peers[0].address.ToStringAddrPort(), addr2.ToStringAddrPort());
        CHECK(!peers[0].received.has_value());
    }

    const auto stale_state{pb.GetStale()};
    CHECK_EQUAL(stale_state.size(), std::remove_cvref_t<decltype(stale_state.size())>{1});
    CHECK_EQUAL(stale_state[0], tx_for_recipient2);

    SetMockTime(Now<NodeSeconds>() + 10h);

    CHECK_EQUAL(pb.GetStale().size(), std::remove_cvref_t<decltype(pb.GetStale().size())>{2});

    CHECK_EQUAL(pb.Remove(tx_for_recipient1).value(), std::remove_cvref_t<decltype(pb.Remove(tx_for_recipient1).value())>{1});
    CHECK(!pb.Remove(tx_for_recipient1).has_value());
    CHECK_EQUAL(pb.Remove(tx_for_recipient2).value(), std::remove_cvref_t<decltype(pb.Remove(tx_for_recipient2).value())>{0});
    CHECK(!pb.Remove(tx_for_recipient2).has_value());

    CHECK_EQUAL(pb.GetBroadcastInfo().size(), std::remove_cvref_t<decltype(pb.GetBroadcastInfo().size())>{0});
    const CService addr_nonexistent{ipv4Addr, 3333};
    CHECK(!pb.PickTxForSend(/*will_send_to_nodeid=*/nonexistent_recipient, /*will_send_to_address=*/addr_nonexistent).has_value());
}

BOOST_AUTO_TEST_CASE(stale_unpicked_tx)
{
    SetMockTime(Now<NodeSeconds>());

    PrivateBroadcast pb;
    const auto tx{MakeDummyTx(/*id=*/42, /*num_witness=*/0)};
    CHECK(pb.Add(tx));

    // Unpicked transactions use the longer INITIAL_STALE_DURATION.
    CHECK_EQUAL(pb.GetStale().size(), std::remove_cvref_t<decltype(pb.GetStale().size())>{0});
    SetMockTime(Now<NodeSeconds>() + PrivateBroadcast::INITIAL_STALE_DURATION - 1min);
    CHECK_EQUAL(pb.GetStale().size(), std::remove_cvref_t<decltype(pb.GetStale().size())>{0});
    SetMockTime(Now<NodeSeconds>() + 2min);
    const auto stale_state{pb.GetStale()};
    CHECK_EQUAL(stale_state.size(), std::remove_cvref_t<decltype(stale_state.size())>{1});
    CHECK_EQUAL(stale_state[0], tx);
}

BOOST_AUTO_TEST_SUITE_END()
