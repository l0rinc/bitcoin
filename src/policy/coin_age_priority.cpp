// Copyright (c) 2012-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/coin_age_priority.h>

#include <coins.h>
#include <consensus/validation.h>
#include <primitives/transaction.h>
#include <txmempool.h>

#include <cassert>

unsigned int CalculateModifiedSize(const CTransaction& tx, unsigned int nTxSize)
{
    // In order to avoid disincentivizing cleaning up the UTXO set we don't count
    // the constant overhead for each txin and up to 110 bytes of scriptSig (which
    // is enough to cover a compressed pubkey p2sh redemption) for priority.
    // Providing any more cleanup incentive than making additional inputs free would
    // risk encouraging people to create junk outputs to redeem later.
    if (nTxSize == 0)
        nTxSize = (GetTransactionWeight(tx) + WITNESS_SCALE_FACTOR - 1) / WITNESS_SCALE_FACTOR;
    for (std::vector<CTxIn>::const_iterator it(tx.vin.begin()); it != tx.vin.end(); ++it)
    {
        unsigned int offset = 41U + std::min(110U, (unsigned int)it->scriptSig.size());
        if (nTxSize > offset)
            nTxSize -= offset;
    }
    return nTxSize;
}

double ComputePriority(const CTransaction& tx, double dPriorityInputs, unsigned int nTxSize)
{
    nTxSize = CalculateModifiedSize(tx, nTxSize);
    if (nTxSize == 0) return 0.0;

    return dPriorityInputs / nTxSize;
}

double GetPriority(const CTransaction &tx, const CCoinsViewCache& view, int nHeight, CAmount &inChainInputValue)
{
    inChainInputValue = 0;
    if (tx.IsCoinBase())
        return 0.0;
    double dResult = 0.0;
    for (const CTxIn& txin : tx.vin)
    {
        const Coin& coin = view.AccessCoin(txin.prevout);
        if (coin.IsSpent()) {
            continue;
        }
        if (coin.nHeight <= nHeight) {
            dResult += (double)(coin.out.nValue) * (nHeight - coin.nHeight);
            inChainInputValue += coin.out.nValue;
        }
    }
    return ComputePriority(tx, dResult);
}

void CTxMemPoolEntry::UpdateCachedPriority(unsigned int currentHeight, CAmount valueInCurrentBlock)
{
    int heightDiff = int(currentHeight) - int(cachedHeight);
    double deltaPriority = ((double)heightDiff*inChainInputValue)/nModSize;
    cachedPriority += deltaPriority;
    cachedHeight = currentHeight;
    inChainInputValue += valueInCurrentBlock;
    assert(MoneyRange(inChainInputValue));
}

struct update_priority
{
    update_priority(unsigned int _height, CAmount _value) :
        height(_height), value(_value)
    {}

    void operator() (CTxMemPoolEntry &e)
    { e.UpdateCachedPriority(height, value); }

    private:
        unsigned int height;
        CAmount value;
};

void CTxMemPool::UpdateDependentPriorities(const CTransaction &tx, unsigned int nBlockHeight, bool addToChain)
{
    LOCK(cs);
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        auto it = mapNextTx.find(COutPoint(tx.GetHash(), i));
        if (it == mapNextTx.end())
            continue;
        mapTx.modify(it->second, update_priority(nBlockHeight, addToChain ? tx.vout[i].nValue : -tx.vout[i].nValue));
    }
}

double
CTxMemPoolEntry::GetPriority(unsigned int currentHeight) const
{
    // This will only return accurate results when currentHeight >= the heights
    // at which all the in-chain inputs of the tx were included in blocks.
    // Typical usage of GetPriority with chainActive.Height() will ensure this.
    int heightDiff = currentHeight - cachedHeight;
    double deltaPriority = ((double)heightDiff*inChainInputValue)/nModSize;
    double dResult = cachedPriority + deltaPriority;
    if (dResult < 0) // This should only happen if it was called with an invalid height
        dResult = 0;
    return dResult;
}
