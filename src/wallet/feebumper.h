// Copyright (c) 2017-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_FEEBUMPER_H
#define BITCOIN_WALLET_FEEBUMPER_H

#include <consensus/consensus.h>
#include <script/interpreter.h>
#include <primitives/transaction.h>

class uint256;
enum class FeeEstimateMode;
struct bilingual_str;

namespace wallet {
class CCoinControl;
class CWallet;
class CWalletTx;

namespace feebumper {

enum class Result
{
    OK,
    INVALID_ADDRESS_OR_KEY,
    INVALID_REQUEST,
    INVALID_PARAMETER,
    WALLET_ERROR,
    MISC_ERROR,
};

//! Return whether transaction can be bumped.
bool TransactionCanBeBumped(const CWallet& wallet, const Txid& txid);

/** Create a bumpfee transaction using fee estimates or an explicit feerate.
 *
 * @param[in] wallet The wallet to use for this bumping
 * @param[in] txid The txid of the transaction to bump
 * @param[in] coin_control A CCoinControl object which provides feerates and other information used for coin selection
 * @param[out] errors Errors
 * @param[out] old_fee The fee the original transaction pays
 * @param[out] new_fee the fee that the bump transaction pays
 * @param[out] mtx The bump transaction itself
 * @param[in] require_mine Whether the original transaction must consist of inputs that can be spent by the wallet
 * @param[in] outputs Vector of new outputs to replace the bumped transaction's outputs
 * @param[in] original_change_index The position of the change output to deduct the fee from in the transaction being bumped
 */
Result CreateRateBumpTransaction(CWallet& wallet,
    const Txid& txid,
    const CCoinControl& coin_control,
    std::vector<bilingual_str>& errors,
    CAmount& old_fee,
    CAmount& new_fee,
    CMutableTransaction& mtx,
    bool require_mine,
    const std::vector<CTxOut>& outputs,
    std::optional<uint32_t> original_change_index = std::nullopt);

//! Sign the new transaction.
//! @return false if signing or finalization failed.
bool SignTransaction(CWallet& wallet, CMutableTransaction& mtx);

//! Commit the bumpfee transaction.
//! @return OK after the transaction is committed to the wallet. If marking the
//! original transaction as replaced fails, errors is populated but the result
//! remains OK.
Result CommitTransaction(CWallet& wallet,
    const Txid& txid,
    CMutableTransaction&& mtx,
    std::vector<bilingual_str>& errors,
    Txid& bumped_txid);

struct SignatureWeights
{
private:
    int m_sigs_count{0};
    int64_t m_sigs_weight{0};

public:
    void AddSigWeight(const size_t weight, const SigVersion sigversion)
    {
        switch (sigversion) {
        case SigVersion::BASE:
            m_sigs_weight += weight * WITNESS_SCALE_FACTOR;
            m_sigs_count += 1 * WITNESS_SCALE_FACTOR;
            break;
        case SigVersion::WITNESS_V0:
            m_sigs_weight += weight;
            m_sigs_count++;
            break;
        case SigVersion::TAPROOT:
        case SigVersion::TAPSCRIPT:
            assert(false);
        }
    }

    int64_t GetWeightDiffToMax() const
    {
        // Note: the witness scaling factor is already accounted for because the count is multiplied by it.
        return (/* max signature size=*/ 72 * m_sigs_count) - m_sigs_weight;
    }
};

class SignatureWeightChecker : public DeferringSignatureChecker
{
private:
    SignatureWeights& m_weights;

public:
    SignatureWeightChecker(SignatureWeights& weights, const BaseSignatureChecker& checker) : DeferringSignatureChecker(checker), m_weights(weights) {}

    bool CheckECDSASignature(const std::vector<unsigned char>& sig, const std::vector<unsigned char>& pubkey, const CScript& script, SigVersion sigversion) const override
    {
        if (m_checker.CheckECDSASignature(sig, pubkey, script, sigversion)) {
            m_weights.AddSigWeight(sig.size(), sigversion);
            return true;
        }
        return false;
    }
};

} // namespace feebumper
} // namespace wallet

#endif // BITCOIN_WALLET_FEEBUMPER_H
