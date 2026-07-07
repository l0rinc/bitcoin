// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <common/signmessage.h>
#include <key_io.h>
#include <pubkey.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/chaintype.h>
#include <util/strencodings.h>

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

void initialize_message()
{
    static ECC_Context ecc_context{};
    SelectParams(ChainType::REGTEST);
}

FUZZ_TARGET(message, .init = initialize_message)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::string random_message = fuzzed_data_provider.ConsumeRandomLengthString(1024);
    {
        CKey private_key = ConsumePrivateKey(fuzzed_data_provider);
        const std::string original_signature{fuzzed_data_provider.ConsumeRandomLengthString(1024)};
        std::string signature{original_signature};
        const bool message_signed = MessageSign(private_key, random_message, signature);
        if (private_key.IsValid()) {
            assert(message_signed);
            const auto signature_bytes{DecodeBase64(signature)};
            assert(signature_bytes);
            assert(signature_bytes->size() == CPubKey::COMPACT_SIGNATURE_SIZE);
            const std::string address{EncodeDestination(PKHash(private_key.GetPubKey().GetID()))};
            const MessageVerificationResult verification_result = MessageVerify(address, signature, random_message);
            assert(verification_result == MessageVerificationResult::OK);
            assert(MessageVerify(address, signature, random_message + '\0') != MessageVerificationResult::OK);
        } else {
            assert(!message_signed);
            assert(signature == original_signature);
        }
    }
    {
        (void)MessageHash(random_message);
        auto address = fuzzed_data_provider.ConsumeRandomLengthString(1024);
        auto signature = fuzzed_data_provider.ConsumeRandomLengthString(1024);
        const MessageVerificationResult verification_result{MessageVerify(address, signature, random_message)};
        const CTxDestination destination{DecodeDestination(address)};
        if (!IsValidDestination(destination)) {
            assert(verification_result == MessageVerificationResult::ERR_INVALID_ADDRESS);
        } else if (!std::get_if<PKHash>(&destination)) {
            assert(verification_result == MessageVerificationResult::ERR_ADDRESS_NO_KEY);
        } else if (!DecodeBase64(signature)) {
            assert(verification_result == MessageVerificationResult::ERR_MALFORMED_SIGNATURE);
        }

        const SigningResult signing_result{fuzzed_data_provider.PickValueInArray({SigningResult::OK, SigningResult::PRIVATE_KEY_NOT_AVAILABLE, SigningResult::SIGNING_FAILED})};
        const std::string signing_result_string{SigningResultString(signing_result)};
        switch (signing_result) {
        case SigningResult::OK:
            assert(signing_result_string == "No error");
            break;
        case SigningResult::PRIVATE_KEY_NOT_AVAILABLE:
            assert(signing_result_string == "Private key not available");
            break;
        case SigningResult::SIGNING_FAILED:
            assert(signing_result_string == "Sign failed");
            break;
        }
    }
}
