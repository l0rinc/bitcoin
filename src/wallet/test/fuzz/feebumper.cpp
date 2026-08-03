// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/wallet.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/time.h>
#include <wallet/context.h>
#include <wallet/feebumper.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

namespace wallet {
namespace {
TestingSetup* g_setup;

void ResetMempool(TestingSetup& setup)
{
    bilingual_str error{};
    setup.m_node.mempool.reset();
    setup.m_node.mempool = std::make_unique<CTxMemPool>(MemPoolOptionsForTest(setup.m_node), error);
    Assert(error.empty());
}

void initialize_setup()
{
    static const auto testing_setup = MakeNoLogFileContext<TestingSetup>();
    g_setup = testing_setup.get();
}

/** Fuzz every precondition arm of feebumper::TransactionCanBeBumped:
 * presence in mapWallet, mined/conflicted depth, m_replaced_by_txid,
 * descendants in the wallet, descendants in the (real, reset) mempool,
 * and foreign (non-wallet) inputs. The expectation is derived ONLY from
 * the fuzz choices; production predicates are queried solely as
 * positive-control harness checks. Revival of the closed-unmerged
 * PR 33916 coverage gap without its production-side virtual (the
 * descendant-in-mempool arm is driven through the real mempool). */
FUZZ_TARGET(wallet_transaction_can_be_bumped, .init = initialize_setup)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    SetMockTime(1231006505); // harness contract: wallet code reads NodeClock; teardown aborts on g_used_system_time otherwise (check_globals.cpp:54)
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};
    ResetMempool(*g_setup);
    FuzzedWallet fuzzed_wallet{
        *g_setup->m_node.chain,
        "fuzzed_wallet_canbebumped",
        "tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK",
    };
    CWallet& wallet{*fuzzed_wallet.wallet};

    CMutableTransaction base;
    const bool foreign_input{fdp.ConsumeBool()};
    if (foreign_input) {
        base.vin.emplace_back(COutPoint{Txid::FromUint256(ConsumeUInt256(fdp)), fdp.ConsumeIntegral<uint32_t>()});
    }
    base.vout.emplace_back(ConsumeMoney(fdp, MAX_MONEY), fuzzed_wallet.GetScriptPubKey(fdp));
    const auto base_ref{MakeTransactionRef(std::move(base))};
    const Txid txid{base_ref->GetHash()};

    bool nonzero_depth{false};
    TxState state{TxStateInactive{/*abandoned=*/false}};
    CallOneOf(fdp,
        [&] { state = TxStateInactive{/*abandoned=*/fdp.ConsumeBool()}; },
        [&] { state = TxStateInMempool{}; },
        [&] {
            const Chainstate& chainstate{g_setup->m_node.chainman->ActiveChainstate()};
            const auto tip{chainstate.m_chain.Tip()};
            state = TxStateConfirmed{tip->GetBlockHash(), tip->nHeight, /*index=*/fdp.ConsumeIntegralInRange<int>(0, 50)};
            nonzero_depth = true;
        },
        [&] {
            // Contract: TxStateBlockConflicted requires conflicting_block_height >= 0
            // (CWallet::GetTxDepthInMainChain asserts it; returns negative depth).
            state = TxStateBlockConflicted{ConsumeUInt256(fdp), /*height=*/fdp.ConsumeIntegralInRange<int>(0, 100'000)};
            nonzero_depth = true;
        });

    const bool in_map{fdp.ConsumeBool()};
    const bool replaced{fdp.ConsumeBool()};
    const bool wallet_desc{fdp.ConsumeBool()};
    const bool mempool_desc{fdp.ConsumeBool()};

    {
        LOCK(wallet.cs_wallet);
        if (in_map) {
            const auto it{wallet.mapWallet.emplace(std::piecewise_construct,
                                                   std::forward_as_tuple(txid),
                                                   std::forward_as_tuple(base_ref, state)).first};
            if (replaced) it->second.m_replaced_by_txid = Txid::FromUint256(ConsumeUInt256(fdp));
            Assert((wallet.GetTxDepthInMainChain(it->second) != 0) == nonzero_depth); // control
        }
        if (wallet_desc) {
            CMutableTransaction desc;
            desc.vin.emplace_back(COutPoint{txid, /*n=*/0});
            desc.vout.emplace_back(ConsumeMoney(fdp, MAX_MONEY / 2), fuzzed_wallet.GetScriptPubKey(fdp));
            Assert(wallet.AddToWallet(MakeTransactionRef(std::move(desc)), TxStateInMempool{})); // AddToWallet updates mapTxSpends (direct mapWallet::emplace does not)
        }
        Assert(wallet.HasWalletSpend(base_ref) == wallet_desc); // control: presence of a wallet-side spender of base's outputs (independent of base's own mapWallet presence)
    }
    if (mempool_desc) {
        auto& pool{*g_setup->m_node.mempool};
        const TestMemPoolEntryHelper entry;
        TryAddToMempool(pool, entry.FromTx(base_ref));
        CMutableTransaction child;
        child.vin.emplace_back(COutPoint{txid, /*n=*/0});
        child.vout.emplace_back(ConsumeMoney(fdp, MAX_MONEY / 2), fuzzed_wallet.GetScriptPubKey(fdp));
        TryAddToMempool(pool, entry.FromTx(MakeTransactionRef(std::move(child))));
        Assert(pool.HasDescendants(txid)); // control
    }

    const bool expect{in_map && !nonzero_depth && !replaced && !wallet_desc && !mempool_desc && !foreign_input};
    Assert(feebumper::TransactionCanBeBumped(wallet, txid) == expect);
    Assert(!feebumper::TransactionCanBeBumped(wallet, Txid::FromUint256(ConsumeUInt256(fdp)))); // never-inserted probe
}
} // namespace
} // namespace wallet
