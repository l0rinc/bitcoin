// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <key_io.h>
#include <test/fuzz/fuzz.h>
#include <util/chaintype.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

void initialize_key_io()
{
    static ECC_Context ecc_context{};
    SelectParams(ChainType::MAIN);
}

FUZZ_TARGET(key_io, .init = initialize_key_io)
{
    const std::string random_string(buffer.begin(), buffer.end());

    std::string error_msg;
    std::vector<int> error_locations{0, 1, 2};
    const CTxDestination destination{DecodeDestination(random_string, error_msg, &error_locations)};
    const bool valid_destination{IsValidDestination(destination)};
    assert(valid_destination == error_msg.empty());
    assert(valid_destination == IsValidDestinationString(random_string));
    if (valid_destination) {
        assert(error_locations.empty());
        const std::string encoded_destination{EncodeDestination(destination)};
        assert(!encoded_destination.empty());
        std::string encoded_error_msg;
        assert(destination == DecodeDestination(encoded_destination, encoded_error_msg));
        assert(encoded_error_msg.empty());
    } else {
        assert(!error_msg.empty());
    }
    for (const int location : error_locations) {
        assert(location >= 0);
        assert(static_cast<size_t>(location) < random_string.size());
    }

    const CKey key = DecodeSecret(random_string);
    if (key.IsValid()) {
        assert(key == DecodeSecret(EncodeSecret(key)));
    }

    const CExtKey ext_key = DecodeExtKey(random_string);
    if (ext_key.key.size() == 32) {
        assert(ext_key == DecodeExtKey(EncodeExtKey(ext_key)));
    }

    const CExtPubKey ext_pub_key = DecodeExtPubKey(random_string);
    if (ext_pub_key.pubkey.size() == CPubKey::COMPRESSED_SIZE) {
        assert(ext_pub_key == DecodeExtPubKey(EncodeExtPubKey(ext_pub_key)));
    }
}
