// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/transaction.h>

#include <streams.h>
#include <test/util/common.h>
#include <wallet/test/wallet_test_fixture.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(wallet_transaction_tests, WalletTestingSetup)

BOOST_AUTO_TEST_CASE(roundtrip)
{
    for (uint8_t hash = 0; hash < 5; ++hash) {
        for (int index = -2; index < 3; ++index) {
            TxState state = TxStateInterpretSerialized(TxStateUnrecognized{uint256{hash}, index});
            BOOST_CHECK_EQUAL(TxStateSerializedBlockHash(state), uint256{hash});
            BOOST_CHECK_EQUAL(TxStateSerializedIndex(state), index);
        }
    }
}

BOOST_AUTO_TEST_CASE(malformed_replacement_txids_throw)
{
    const CTransactionRef tx{MakeTransactionRef(CMutableTransaction{})};
    const TxState state{TxStateInactive{}};

    for (const char* key : {"replaces_txid", "replaced_by_txid"}) {
        DataStream stream{};
        stream << TX_WITH_WITNESS(tx)
               << TxStateSerializedBlockHash(state)
               << std::vector<uint8_t>{}
               << TxStateSerializedIndex(state)
               << std::vector<uint8_t>{}
               << std::map<std::string, std::string>{{key, "not-a-txid"}}
               << std::vector<std::pair<std::string, std::string>>{}
               << uint32_t{0}
               << 0U
               << false
               << false;

        CWalletTx wtx{nullptr, TxStateInactive{}};
        BOOST_CHECK_THROW(stream >> wtx, std::runtime_error);
    }
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
