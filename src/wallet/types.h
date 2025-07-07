// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//! @file wallet/types.h is a home for public enum and struct type definitions
//! that are used by internally by wallet code, but also used externally by node
//! or GUI code.
//!
//! This file is intended to define only simple types that do not have external
//! dependencies. More complicated public wallet types like CCoinControl should
//! be defined in dedicated header files.

#ifndef BITCOIN_WALLET_TYPES_H
#define BITCOIN_WALLET_TYPES_H

#include <policy/fees/block_policy_estimator.h>
#include <util/translation.h>

#include <type_traits>

namespace wallet {
/**
 * IsMine() return codes, which depend on ScriptPubKeyMan implementation.
 * Not every ScriptPubKeyMan covers all types, please refer to
 * https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.21.0.md#ismine-semantics
 * for better understanding.
 *
 * For LegacyScriptPubKeyMan,
 * ISMINE_NO: the scriptPubKey is not in the wallet;
 * ISMINE_WATCH_ONLY: the scriptPubKey has been imported into the wallet;
 * ISMINE_SPENDABLE: the scriptPubKey corresponds to an address owned by the wallet user (can spend with the private key);
 * ISMINE_USED: the scriptPubKey corresponds to a used address owned by the wallet user;
 * ISMINE_ALL: all ISMINE flags except for USED;
 * ISMINE_ALL_USED: all ISMINE flags including USED;
 * ISMINE_ENUM_ELEMENTS: the number of isminetype enum elements.
 *
 * For DescriptorScriptPubKeyMan and future ScriptPubKeyMan,
 * ISMINE_NO: the scriptPubKey is not in the wallet;
 * ISMINE_SPENDABLE: the scriptPubKey matches a scriptPubKey in the wallet.
 * ISMINE_USED: the scriptPubKey corresponds to a used address owned by the wallet user.
 */
enum isminetype : unsigned int {
    ISMINE_NO         = 0,
    ISMINE_WATCH_ONLY = 1 << 0,
    ISMINE_SPENDABLE  = 1 << 1,
    ISMINE_USED       = 1 << 2,
    ISMINE_ALL        = ISMINE_WATCH_ONLY | ISMINE_SPENDABLE,
    ISMINE_ALL_USED   = ISMINE_ALL | ISMINE_USED,
    ISMINE_ENUM_ELEMENTS,
};
/** used for bitflags of isminetype */
using isminefilter = std::underlying_type<isminetype>::type;

/**
 * Address purpose field that has been been stored with wallet sending and
 * receiving addresses since BIP70 payment protocol support was added in
 * https://github.com/bitcoin/bitcoin/pull/2539. This field is not currently
 * used for any logic inside the wallet, but it is still shown in RPC and GUI
 * interfaces and saved for new addresses. It is basically redundant with an
 * address's IsMine() result.
 */
enum class AddressPurpose {
    RECEIVE,
    SEND,
    REFUND, //!< Never set in current code may be present in older wallet databases
};

struct CreatedTransactionResult
{
    CTransactionRef tx;
    CAmount fee;
    FeeCalculation fee_calc;
    std::optional<unsigned int> change_pos;

    CreatedTransactionResult(CTransactionRef _tx, CAmount _fee, std::optional<unsigned int> _change_pos, const FeeCalculation& _fee_calc)
            : tx(_tx), fee(_fee), fee_calc(_fee_calc), change_pos(_change_pos) {}
};

//! Machine-readable wallet error codes.
//!
//! @note Add new codes only when callers need to handle the condition
//! differently. For errors that should only be displayed to the user, use
//! WalletErrorCode::GenericError and provide the user-facing details in WalletError::message.
enum class WalletErrorCode {
    //! Generic wallet error. Callers may present the accompanying message to
    //! the user.
    GenericError,

    //! The wallet is locked and the operation requires access to private keys.
    //! Callers may ask the user to unlock the wallet and retry the operation.
    UnlockNeeded,
};

//! Wallet-layer error with both programmatic and user-facing information.
//!
//! Wallet methods should return a specific WalletErrorCode only when callers
//! can handle that condition differently. Otherwise, use WalletErrorCode::GenericError
//! and describe the failure in `message`.
struct WalletError {
    //! Machine-readable error code for callers that need programmatic handling.
    WalletErrorCode code;
    //! User-facing translated error message
    bilingual_str message;
};

} // namespace wallet

#endif // BITCOIN_WALLET_TYPES_H
