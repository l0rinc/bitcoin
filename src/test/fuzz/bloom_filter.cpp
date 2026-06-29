// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/bloom.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/solver.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <uint256.h>

#include <cassert>
#include <limits>
#include <optional>
#include <vector>

namespace {
bool ScriptMatchesFilter(const CBloomFilter& filter, const CScript& script)
{
    CScript::const_iterator pc{script.begin()};
    std::vector<unsigned char> data;
    while (pc < script.end()) {
        opcodetype opcode;
        if (!script.GetOp(pc, opcode, data)) break;
        if (!data.empty() && filter.contains(data)) return true;
    }
    return false;
}

struct BloomUpdateOracle {
    bool relevant{false};
    std::vector<COutPoint> updated_outpoints;
};

BloomUpdateOracle PredictIsRelevantAndUpdate(CBloomFilter filter, const CTransaction& tx, const unsigned char flags)
{
    BloomUpdateOracle oracle;
    const Txid& txid{tx.GetHash()};
    if (filter.contains(txid.ToUint256())) {
        oracle.relevant = true;
    }

    for (size_t i{0}; i < tx.vout.size(); ++i) {
        const CTxOut& txout{tx.vout[i]};
        if (!ScriptMatchesFilter(filter, txout.scriptPubKey)) continue;
        oracle.relevant = true;

        const COutPoint outpoint{txid, static_cast<uint32_t>(i)};
        const auto update_flags{flags & BLOOM_UPDATE_MASK};
        if (update_flags == BLOOM_UPDATE_ALL) {
            filter.insert(outpoint);
            oracle.updated_outpoints.push_back(outpoint);
        } else if (update_flags == BLOOM_UPDATE_P2PUBKEY_ONLY) {
            std::vector<std::vector<unsigned char>> solutions;
            const TxoutType type{Solver(txout.scriptPubKey, solutions)};
            if (type == TxoutType::PUBKEY || type == TxoutType::MULTISIG) {
                filter.insert(outpoint);
                oracle.updated_outpoints.push_back(outpoint);
            }
        }
    }

    if (oracle.relevant) return oracle;

    for (const CTxIn& txin : tx.vin) {
        if (filter.contains(txin.prevout) || ScriptMatchesFilter(filter, txin.scriptSig)) {
            oracle.relevant = true;
            return oracle;
        }
    }

    return oracle;
}
} // namespace

FUZZ_TARGET(bloom_filter)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    bool good_data{true};
    const auto update_flags{static_cast<unsigned char>(fuzzed_data_provider.PickValueInArray({BLOOM_UPDATE_NONE, BLOOM_UPDATE_ALL, BLOOM_UPDATE_P2PUBKEY_ONLY, BLOOM_UPDATE_MASK}))};

    CBloomFilter bloom_filter{
        fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1, 10000000),
        1.0 / fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1, std::numeric_limits<unsigned int>::max()),
        fuzzed_data_provider.ConsumeIntegral<unsigned int>(),
        update_flags};
    LIMITED_WHILE(good_data && fuzzed_data_provider.remaining_bytes() > 0, 10'000)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                const std::vector<unsigned char> b = ConsumeRandomLengthByteVector(fuzzed_data_provider);
                (void)bloom_filter.contains(b);
                bloom_filter.insert(b);
                const bool present = bloom_filter.contains(b);
                assert(present);
            },
            [&] {
                const std::optional<COutPoint> out_point = ConsumeDeserializable<COutPoint>(fuzzed_data_provider);
                if (!out_point) {
                    good_data = false;
                    return;
                }
                (void)bloom_filter.contains(*out_point);
                bloom_filter.insert(*out_point);
                const bool present = bloom_filter.contains(*out_point);
                assert(present);
            },
            [&] {
                const std::optional<uint256> u256 = ConsumeDeserializable<uint256>(fuzzed_data_provider);
                if (!u256) {
                    good_data = false;
                    return;
                }
                (void)bloom_filter.contains(*u256);
                bloom_filter.insert(*u256);
                const bool present = bloom_filter.contains(*u256);
                assert(present);
            },
            [&] {
                const std::optional<CMutableTransaction> mut_tx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
                if (!mut_tx) {
                    good_data = false;
                    return;
                }
                const CTransaction tx{*mut_tx};
                const BloomUpdateOracle oracle{PredictIsRelevantAndUpdate(bloom_filter, tx, update_flags)};
                const bool relevant{bloom_filter.IsRelevantAndUpdate(tx)};
                assert(relevant == oracle.relevant);
                for (const COutPoint& outpoint : oracle.updated_outpoints) {
                    assert(bloom_filter.contains(outpoint));
                }
            });
        (void)bloom_filter.IsWithinSizeConstraints();
    }
}
