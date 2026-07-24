// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <common/signmessage.h>
#include <hash.h>
#include <key_io.h>
#include <pubkey.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/chaintype.h>
#include <util/strencodings.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

void initialize_message()
{
    static ECC_Context ecc_context{};
    SelectParams(ChainType::REGTEST);
}

static void AssertMessageSignatureContracts(const CKey& private_key, const std::string& message, const std::string& signature)
{
    assert(private_key.IsValid());

    const CPubKey public_key = private_key.GetPubKey();
    const std::string address = EncodeDestination(PKHash(public_key.GetID()));
    const auto signature_bytes = DecodeBase64(signature);
    assert(signature_bytes);
    assert(signature_bytes->size() == CPubKey::COMPACT_SIGNATURE_SIZE);

    CPubKey recovered_key;
    assert(recovered_key.RecoverCompact(MessageHash(message), *signature_bytes));
    assert(recovered_key == public_key);
    assert(MessageVerify(address, signature, message) == MessageVerificationResult::OK);

    std::array<unsigned char, 32> alternate_key_data{};
    alternate_key_data.back() = 1;
    CKey alternate_private_key;
    alternate_private_key.Set(alternate_key_data.begin(), alternate_key_data.end(), private_key.IsCompressed());
    if (alternate_private_key.GetPubKey() == public_key) {
        alternate_key_data.back() = 2;
        alternate_private_key.Set(alternate_key_data.begin(), alternate_key_data.end(), private_key.IsCompressed());
    }
    assert(alternate_private_key.IsValid());
    assert(alternate_private_key.GetPubKey() != public_key);
    const std::string alternate_address = EncodeDestination(PKHash(alternate_private_key.GetPubKey().GetID()));
    assert(MessageVerify(alternate_address, signature, message) == MessageVerificationResult::ERR_NOT_SIGNED);

    std::string altered_message{message};
    altered_message.push_back('\0');
    assert(MessageVerify(address, signature, altered_message) != MessageVerificationResult::OK);
    assert(MessageVerify(EncodeDestination(WitnessV0KeyHash{}), signature, message) == MessageVerificationResult::ERR_ADDRESS_NO_KEY);
    assert(MessageVerify("", signature, message) == MessageVerificationResult::ERR_INVALID_ADDRESS);
    assert(MessageVerify(address, "!", message) == MessageVerificationResult::ERR_MALFORMED_SIGNATURE);
}

FUZZ_TARGET(message, .init = initialize_message)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::string random_message = fuzzed_data_provider.ConsumeRandomLengthString(1024);

    std::vector<unsigned char> serialized_message;
    VectorWriter message_writer{serialized_message, 0};
    message_writer << MESSAGE_MAGIC << random_message;
    assert(Hash(serialized_message) == MessageHash(random_message));

    {
        CKey private_key = ConsumePrivateKey(fuzzed_data_provider);
        std::string signature{"unchanged"};
        const bool message_signed = MessageSign(private_key, random_message, signature);
        if (private_key.IsValid()) {
            assert(message_signed);
            AssertMessageSignatureContracts(private_key, random_message, signature);

        } else {
            assert(!message_signed);
            assert(signature == "unchanged");
        }
    }
    {
        (void)MessageHash(random_message);
        auto address = fuzzed_data_provider.ConsumeRandomLengthString(1024);
        auto signature = fuzzed_data_provider.ConsumeRandomLengthString(1024);
        (void)MessageVerify(address, signature, random_message);
        const SigningResult signing_result = fuzzed_data_provider.PickValueInArray({SigningResult::OK, SigningResult::PRIVATE_KEY_NOT_AVAILABLE, SigningResult::SIGNING_FAILED});
        assert(!SigningResultString(signing_result).empty());
    }
}
