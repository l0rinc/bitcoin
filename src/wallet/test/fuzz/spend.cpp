// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <consensus/tx_check.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/wallet.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/time.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/context.h>
#include <wallet/spend.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

#include <set>

using util::ToString;

namespace wallet {
namespace {
const TestingSetup* g_setup;

void initialize_setup()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

COutPoint AddConfirmedTransaction(CWallet& wallet, Chainstate& chainstate, CMutableTransaction tx)
{
    LOCK(wallet.cs_wallet);
    const auto txid{tx.GetHash()};
    const auto ret{wallet.mapWallet.emplace(std::piecewise_construct, std::forward_as_tuple(txid),
                                            std::forward_as_tuple(MakeTransactionRef(std::move(tx)),
                                                                  TxStateConfirmed{chainstate.m_chain.Tip()->GetBlockHash(), chainstate.m_chain.Height(), /*index=*/0}))};
    assert(ret.second);
    wallet.RefreshTXOsFromTx(ret.first->second);
    return COutPoint{txid, 0};
}

void AssertCreatedTransaction(const CWallet& wallet, const std::vector<CRecipient>& recipients,
                              const CCoinControl& coin_control, const CreatedTransactionResult& result)
{
    assert(result.tx);
    const CTransaction& tx{*result.tx};
    TxValidationState state;
    assert(CheckTransaction(tx, state) && state.IsValid());
    assert(!tx.vin.empty());
    assert(!tx.vout.empty());

    LOCK(wallet.cs_wallet);
    std::set<COutPoint> spent_outpoints;
    CAmount input_value{0};
    for (const CTxIn& txin : tx.vin) {
        assert(spent_outpoints.insert(txin.prevout).second);
        const auto it{wallet.mapWallet.find(txin.prevout.hash)};
        assert(it != wallet.mapWallet.end());
        assert(txin.prevout.n < it->second.tx->vout.size());
        const CTxOut& prevout{it->second.tx->vout[txin.prevout.n]};
        assert(wallet.IsMine(prevout));
        input_value += prevout.nValue;
        assert(MoneyRange(input_value));
    }

    const CAmount output_value{CalculateOutputValue(tx)};
    assert(MoneyRange(output_value));
    assert(result.fee == input_value - output_value);
    assert(result.fee >= 0);
    assert(MoneyRange(result.fee));

    if (result.change_pos) {
        assert(*result.change_pos < tx.vout.size());
        assert(tx.vout[*result.change_pos].nValue > 0);
        if (std::get_if<CNoDestination>(&coin_control.destChange)) {
            assert(wallet.IsMine(tx.vout[*result.change_pos].scriptPubKey));
        } else {
            assert(tx.vout[*result.change_pos].scriptPubKey == GetScriptForDestination(coin_control.destChange));
        }
    }

    size_t txout_index{0};
    for (const CRecipient& recipient : recipients) {
        if (result.change_pos && txout_index == *result.change_pos) ++txout_index;
        assert(txout_index < tx.vout.size());
        const CTxOut& txout{tx.vout[txout_index]};
        assert(txout.scriptPubKey == GetScriptForDestination(recipient.dest));
        if (!recipient.fSubtractFeeFromAmount) {
            assert(txout.nValue == recipient.nAmount);
        }
        ++txout_index;
    }
    if (result.change_pos && txout_index == *result.change_pos) ++txout_index;
    assert(txout_index == tx.vout.size());
}

FUZZ_TARGET(wallet_create_transaction, .init = initialize_setup)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};
    const auto& node = g_setup->m_node;
    Chainstate& chainstate{node.chainman->ActiveChainstate()};
    ArgsManager& args = *node.args;
    const CAmount dust_relay_fee{fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(0, MAX_MONEY)};
    args.ForceSetArg("-dustrelayfee", ToString(dust_relay_fee));
    FuzzedWallet fuzzed_wallet{
        *g_setup->m_node.chain,
        "fuzzed_wallet_a",
        "tprv8ZgxMBicQKsPd1QwsGgzfu2pcPYbBosZhJknqreRHgsWx32nNEhMjGQX2cgFL8n6wz9xdDYwLcs78N4nsCo32cxEX8RBtwGsEGgybLiQJfk",
    };
    const CTxDestination funding_destination{*Assert(fuzzed_wallet.wallet->GetNewDestination(OutputType::BECH32, ""))};
    CMutableTransaction funding_tx;
    funding_tx.vout.emplace_back(MAX_MONEY, GetScriptForDestination(funding_destination));
    const COutPoint funding_outpoint{AddConfirmedTransaction(*fuzzed_wallet.wallet, chainstate, std::move(funding_tx))};

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
        (void)AddConfirmedTransaction(*fuzzed_wallet.wallet, chainstate, std::move(tx));
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
    const auto result{CreateTransaction(*fuzzed_wallet.wallet, recipients, change_pos, coin_control)};
    if (result) AssertCreatedTransaction(*fuzzed_wallet.wallet, recipients, coin_control, *result);

    args.ForceSetArg("-dustrelayfee", "0");
    const CTxDestination fixture_destination{*Assert(fuzzed_wallet.wallet->GetNewDestination(OutputType::BECH32, ""))};
    const std::vector<CRecipient> fixture_recipients{{fixture_destination, COIN, /*fSubtractFeeFromAmount=*/false}};
    CCoinControl fixture_coin_control;
    fixture_coin_control.Select(funding_outpoint);
    const auto fixture_result{CreateTransaction(*fuzzed_wallet.wallet, fixture_recipients, std::nullopt, fixture_coin_control, /*sign=*/false)};
    assert(fixture_result);
    AssertCreatedTransaction(*fuzzed_wallet.wallet, fixture_recipients, fixture_coin_control, *fixture_result);
}
} // namespace
} // namespace wallet
