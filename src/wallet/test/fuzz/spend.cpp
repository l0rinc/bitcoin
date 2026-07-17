// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/wallet.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/moneystr.h>
#include <util/time.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/context.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

#include <algorithm>
#include <memory>
#include <vector>

using util::ToString;

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

FUZZ_TARGET(wallet_create_transaction, .init = initialize_setup)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};
    auto& node = g_setup->m_node;
    Chainstate& chainstate{node.chainman->ActiveChainstate()};
    ArgsManager& args = *node.args;
    args.ForceSetArg("-dustrelayfee", FormatMoney(fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, MAX_MONEY)));
    ResetMempool(*g_setup);
    FuzzedWallet fuzzed_wallet{
        *g_setup->m_node.chain,
        "fuzzed_wallet_a",
        "tprv8ZgxMBicQKsPd1QwsGgzfu2pcPYbBosZhJknqreRHgsWx32nNEhMjGQX2cgFL8n6wz9xdDYwLcs78N4nsCo32cxEX8RBtwGsEGgybLiQJfk",
    };

    CCoinControl coin_control;
    if (fuzzed_data_provider.ConsumeBool()) coin_control.m_version = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
    coin_control.m_avoid_partial_spends = fuzzed_data_provider.ConsumeBool();
    coin_control.m_include_unsafe_inputs = fuzzed_data_provider.ConsumeBool();
    if (fuzzed_data_provider.ConsumeBool()) coin_control.m_confirm_target = fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, 999'000);
    coin_control.destChange = fuzzed_data_provider.ConsumeBool() ? fuzzed_wallet.GetDestination(fuzzed_data_provider) : ConsumeTxDestination(fuzzed_data_provider);
    if (fuzzed_data_provider.ConsumeBool()) coin_control.m_change_type = fuzzed_data_provider.PickValueInArray(OUTPUT_TYPES);
    if (fuzzed_data_provider.ConsumeBool()) coin_control.m_feerate = CFeeRate(ConsumeMoney(fuzzed_data_provider, /*max=*/COIN));
    coin_control.m_allow_other_inputs = fuzzed_data_provider.ConsumeBool();
    coin_control.m_locktime = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
    coin_control.fOverrideFeeRate = fuzzed_data_provider.ConsumeBool();

    int next_locktime{0};
    CAmount all_values{0};
    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
        CMutableTransaction tx;
        tx.nLockTime = next_locktime++;
        tx.vout.resize(1);
        CAmount n_value{ConsumeMoney(fuzzed_data_provider)};
        all_values += n_value;
        if (all_values > MAX_MONEY) return;
        tx.vout[0].nValue = n_value;
        tx.vout[0].scriptPubKey = GetScriptForDestination(fuzzed_wallet.GetDestination(fuzzed_data_provider));
        LOCK(fuzzed_wallet.wallet->cs_wallet);
        auto txid{tx.GetHash()};
        auto ret{fuzzed_wallet.wallet->mapWallet.emplace(std::piecewise_construct, std::forward_as_tuple(txid), std::forward_as_tuple(MakeTransactionRef(std::move(tx)), TxStateConfirmed{chainstate.m_chain.Tip()->GetBlockHash(), chainstate.m_chain.Height(), /*index=*/0}))};
        assert(ret.second);
    }

    if (fuzzed_data_provider.ConsumeBool()) {
        // Add a confirmed wallet transaction and an in-mempool child with two
        // wallet outputs. This reaches the shared-ancestry bump-fee path that
        // cannot be reached by the confirmed-only synthetic transactions above.
        CMutableTransaction funding;
        funding.nLockTime = next_locktime++;
        funding.vout.emplace_back(fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(COIN, 10 * COIN), fuzzed_wallet.GetScriptPubKey(fuzzed_data_provider));
        const auto funding_tx = MakeTransactionRef(std::move(funding));
        const auto tip = chainstate.m_chain.Tip();
        assert(fuzzed_wallet.wallet->AddToWallet(funding_tx, TxStateConfirmed{tip->GetBlockHash(), tip->nHeight, /*index=*/0}));

        CMutableTransaction unconfirmed;
        unconfirmed.nLockTime = next_locktime++;
        unconfirmed.vin.emplace_back(COutPoint{funding_tx->GetHash(), 0});
        std::vector<CAmount> unconfirmed_values;
        for (int i = 0; i < 2; ++i) {
            unconfirmed_values.push_back(fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(COIN, 10 * COIN));
            unconfirmed.vout.emplace_back(unconfirmed_values.back(), fuzzed_wallet.GetScriptPubKey(fuzzed_data_provider));
        }
        const auto unconfirmed_tx = MakeTransactionRef(std::move(unconfirmed));
        TestMemPoolEntryHelper entry;
        TryAddToMempool(*node.mempool, entry.Fee(fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(1, COIN)).FromTx(unconfirmed_tx));
        assert(node.mempool->exists(unconfirmed_tx->GetHash()));
        assert(fuzzed_wallet.wallet->AddToWallet(unconfirmed_tx, TxStateInMempool{}));

        // Remove the other synthetic wallet outputs from consideration so the
        // follow-up transaction must select both outputs sharing this parent.
        {
            LOCK(fuzzed_wallet.wallet->cs_wallet);
            for (const auto& [outpoint, _] : fuzzed_wallet.wallet->GetTXOs()) {
                if (outpoint.hash != unconfirmed_tx->GetHash()) {
                    fuzzed_wallet.wallet->LockCoin(outpoint, /*persist=*/false);
                }
            }

            // Exercise the public AvailableCoins contract without coin control while
            // an unconfirmed wallet transaction is present.
            const auto available = AvailableCoins(*fuzzed_wallet.wallet, /*coinControl=*/nullptr, CFeeRate{1}, {}).All();
            assert(available.size() == 2);
            assert(std::all_of(available.begin(), available.end(), [&](const COutput& output) {
                return output.outpoint.hash == unconfirmed_tx->GetHash();
            }));
        }

        const CAmount followup_target{std::max(unconfirmed_values[0], unconfirmed_values[1]) + COIN / 2};
        CCoinControl followup_coin_control;
        followup_coin_control.m_allow_other_inputs = true;
        followup_coin_control.m_feerate = CFeeRate{1};
        followup_coin_control.fOverrideFeeRate = true;
        followup_coin_control.m_change_type = OutputType::BECH32;
        const CRecipient followup_recipient{fuzzed_wallet.GetDestination(fuzzed_data_provider), followup_target, /*fSubtractFeeFromAmount=*/false};
        const auto followup = CreateTransaction(*fuzzed_wallet.wallet, {followup_recipient}, /*change_pos=*/std::nullopt, followup_coin_control);
        if (followup) {
            bool selected_first{false};
            bool selected_second{false};
            for (const auto& txin : followup->tx->vin) {
                selected_first |= txin.prevout == COutPoint{unconfirmed_tx->GetHash(), 0};
                selected_second |= txin.prevout == COutPoint{unconfirmed_tx->GetHash(), 1};
            }
            assert(selected_first);
            assert(selected_second);
        }
    }

    std::vector<CRecipient> recipients;
    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 100) {
        CTxDestination destination;
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                destination = fuzzed_wallet.GetDestination(fuzzed_data_provider);
            },
            [&] {
                CScript script;
                script << OP_RETURN;
                destination = CNoDestination{script};
            },
            [&] {
                destination = ConsumeTxDestination(fuzzed_data_provider);
            }
        );
        recipients.push_back({destination,
                              /*nAmount=*/ConsumeMoney(fuzzed_data_provider),
                              /*fSubtractFeeFromAmount=*/fuzzed_data_provider.ConsumeBool()});
    }

    std::optional<unsigned int> change_pos;
    if (fuzzed_data_provider.ConsumeBool()) change_pos = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
    (void)CreateTransaction(*fuzzed_wallet.wallet, recipients, change_pos, coin_control);
}
} // namespace
} // namespace wallet
