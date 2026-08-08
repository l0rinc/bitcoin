// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <index/txindex.h>
#include <merkleblock.h>
#include <node/blockstorage.h>
#include <node/context.h>
#include <primitives/transaction.h>
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <univalue.h>
#include <util/strencodings.h>
#include <validation.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <set>

using node::GetTransaction;

//! Find the block creating the first unspent output of any of `txids`, scanning the UTXO set
//! round-robin (every txid at output 0, then every txid at output 1, ...) so that one shared
//! MAX_OUTPUTS_PER_BLOCK budget cannot be exhausted by the first txid alone. `cs_main` is
//! released between batches, so the scan does not observe a single chainstate snapshot.
static const CBlockIndex* FindBlockByUTXO(ChainstateManager& chainman, const std::set<Txid>& txids, const std::function<void()>& interruption_point)
{
    static constexpr uint64_t UTXO_LOOKUP_BATCH_SIZE{1'000};
    auto txid{txids.begin()};
    uint32_t output_index{0};
    for (uint64_t lookups{0}; lookups < MAX_OUTPUTS_PER_BLOCK;) {
        {
            LOCK(cs_main);
            Chainstate& active_chainstate{chainman.ActiveChainstate()};
            const CCoinsViewCache& coins{active_chainstate.CoinsTip()};
            for (const uint64_t batch_end{std::min(lookups + UTXO_LOOKUP_BATCH_SIZE, MAX_OUTPUTS_PER_BLOCK)}; lookups < batch_end; ++lookups) {
                const Coin& coin{coins.AccessCoin(COutPoint{*txid, output_index})};
                if (!coin.IsSpent()) return active_chainstate.m_chain[coin.nHeight];
                if (++txid == txids.end()) {
                    txid = txids.begin();
                    ++output_index;
                }
            }
        }
        interruption_point();
    }
    return nullptr;
}

static RPCMethod gettxoutproof()
{
    return RPCMethod{
        "gettxoutproof",
        "Returns a hex-encoded proof that \"txid\" was included in a block.\n"
        "\nNOTE: By default this function only works sometimes. This is when there is an\n"
        "unspent output in the utxo for this transaction. A request for multiple\n"
        "transactions may fail even when one has an unspent output because all txids\n"
        "share one UTXO lookup budget. To make it always work,\n"
        "you need to maintain a transaction index, using the -txindex command line option or\n"
        "specify the block in which the transaction is included manually (by blockhash).\n",
        {
            {"txids", RPCArg::Type::ARR, RPCArg::Optional::NO, "The txids to filter",
                {
                    {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "A transaction hash"},
                },
            },
            {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "If specified, looks for txid in the block with this hash"},
        },
        RPCResult{
            RPCResult::Type::STR, "data", "A string that is a serialized, hex-encoded data for the proof."
        },
        RPCExamples{""},
        [](const RPCMethod& self, const JSONRPCRequest& request) -> UniValue
        {
            std::set<Txid> setTxids;
            UniValue txids = request.params[0].get_array();
            if (txids.empty()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Parameter 'txids' cannot be empty");
            }
            for (unsigned int idx = 0; idx < txids.size(); idx++) {
                auto ret{setTxids.insert(Txid::FromUint256(ParseHashV(txids[idx], "txid")))};
                if (!ret.second) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, duplicated txid: ") + txids[idx].get_str());
                }
            }

            const CBlockIndex* pblockindex = nullptr;
            uint256 hashBlock;
            const node::NodeContext& node{EnsureAnyNodeContext(request.context)};
            ChainstateManager& chainman{EnsureChainman(node)};
            if (!request.params[1].isNull()) {
                LOCK(cs_main);
                hashBlock = ParseHashV(request.params[1], "blockhash");
                pblockindex = chainman.m_blockman.LookupBlockIndex(hashBlock);
                if (!pblockindex) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
                }
            // Allow txindex to catch up before we acquire cs_main, and only scan the UTXO set if it cannot answer.
            } else if (!g_txindex || !g_txindex->BlockUntilSyncedToCurrentChain()) {
                pblockindex = FindBlockByUTXO(chainman, setTxids, node.rpc_interruption_point);
            }

            if (pblockindex == nullptr) {
                const CTransactionRef tx = GetTransaction(/*block_index=*/nullptr, /*mempool=*/nullptr, *setTxids.begin(), chainman.m_blockman, hashBlock);
                if (!tx || hashBlock.IsNull()) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not yet in block");
                }

                LOCK(cs_main);
                pblockindex = chainman.m_blockman.LookupBlockIndex(hashBlock);
                if (!pblockindex) {
                    throw JSONRPCError(RPC_INTERNAL_ERROR, "Transaction index corrupt");
                }
            }

            {
                LOCK(cs_main);
                CheckBlockDataAvailability(chainman.m_blockman, *pblockindex, /*check_for_undo=*/false);
            }
            CBlock block;
            if (!chainman.m_blockman.ReadBlock(block, *pblockindex)) {
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Can't read block from disk");
            }

            unsigned int ntxFound = 0;
            for (const auto& tx : block.vtx) {
                if (setTxids.contains(tx->GetHash())) {
                    ntxFound++;
                }
            }
            if (ntxFound != setTxids.size()) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Not all transactions found in specified or retrieved block");
            }

            DataStream ssMB{};
            CMerkleBlock mb(block, setTxids);
            ssMB << mb;
            std::string strHex = HexStr(ssMB);
            return strHex;
        },
    };
}

static RPCMethod verifytxoutproof()
{
    return RPCMethod{
        "verifytxoutproof",
        "Verifies that a proof points to a transaction in a block, returning the transaction it commits to\n"
        "and throwing an RPC error if the block is not in our best chain\n",
        {
            {"proof", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hex-encoded proof generated by gettxoutproof"},
        },
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::STR_HEX, "txid", "The txid(s) which the proof commits to, or empty array if the proof cannot be validated."},
            }
        },
        RPCExamples{""},
        [](const RPCMethod& self, const JSONRPCRequest& request) -> UniValue
        {
            CMerkleBlock merkleBlock;
            SpanReader{ParseHexV(request.params[0], "proof")} >> merkleBlock;

            UniValue res(UniValue::VARR);

            std::vector<Txid> vMatch;
            std::vector<unsigned int> vIndex;
            if (merkleBlock.txn.ExtractMatches(vMatch, vIndex) != merkleBlock.header.hashMerkleRoot)
                return res;

            ChainstateManager& chainman = EnsureAnyChainman(request.context);
            LOCK(cs_main);

            const CBlockIndex* pindex = chainman.m_blockman.LookupBlockIndex(merkleBlock.header.GetHash());
            if (!pindex || !chainman.ActiveChain().Contains(*pindex) || pindex->nTx == 0) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found in chain");
            }

            // Check if proof is valid, only add results if so
            if (pindex->nTx == merkleBlock.txn.GetNumTransactions()) {
                for (const auto& txid : vMatch) {
                    res.push_back(txid.GetHex());
                }
            }

            return res;
        },
    };
}

void RegisterTxoutProofRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[]{
        {"blockchain", &gettxoutproof},
        {"blockchain", &verifytxoutproof},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
