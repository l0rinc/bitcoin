// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <key.h>
#include <key_io.h>
#include <outputtype.h>
#include <policy/policy.h>
#include <pubkey.h>
#include <rpc/util.h>
#include <script/keyorigin.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/solver.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>
#include <util/chaintype.h>
#include <util/strencodings.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <optional>
#include <string>
#include <vector>
#include <test/util/check.h>

void initialize_key()
{
    static ECC_Context ecc_context{};
    SelectParams(ChainType::REGTEST);
}

FUZZ_TARGET(key, .init = initialize_key)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    const CKey key = [&] {
        CKey k;
        k.Set(buffer.begin(), buffer.end(), true);
        return k;
    }();
    if (!key.IsValid()) {
        return;
    }

    {
        CHECK(key.begin() + key.size() == key.end());
        CHECK(key.IsCompressed());
        CHECK(key.size() == 32);
        CHECK(DecodeSecret(EncodeSecret(key)) == key);
    }

    {
        CKey invalid_key;
        CHECK(!(invalid_key == key));
        CHECK(!invalid_key.IsCompressed());
        CHECK(!invalid_key.IsValid());
        CHECK(invalid_key.size() == 0);
    }

    {
        CKey uncompressed_key;
        uncompressed_key.Set(buffer.begin(), buffer.end(), false);
        CHECK(!(uncompressed_key == key));
        CHECK(!uncompressed_key.IsCompressed());
        CHECK(key.size() == 32);
        CHECK(uncompressed_key.begin() + uncompressed_key.size() == uncompressed_key.end());
        CHECK(uncompressed_key.IsValid());
    }

    {
        CKey copied_key;
        copied_key.Set(key.begin(), key.end(), key.IsCompressed());
        CHECK(copied_key == key);
    }

    const uint256 random_uint256 = Hash(buffer);

    {
        CKey child_key;
        ChainCode child_chaincode;
        const bool ok = key.Derive(child_key, child_chaincode, 0, random_uint256);
        CHECK(ok);
        CHECK(child_key.IsValid());
        CHECK(!(child_key == key));
        CHECK(child_chaincode != random_uint256);
    }

    const CPubKey pubkey = key.GetPubKey();

    {
        CHECK(pubkey.size() == 33);
        CHECK(key.VerifyPubKey(pubkey));
        CHECK(pubkey.GetHash() != random_uint256);
        CHECK(pubkey.begin() + pubkey.size() == pubkey.end());
        CHECK(pubkey.data() == pubkey.begin());
        CHECK(pubkey.IsCompressed());
        CHECK(pubkey.IsValid());
        CHECK(pubkey.IsFullyValid());
        CHECK(HexToPubKey(HexStr(pubkey)) == pubkey);
    }

    {
        DataStream data_stream{};
        pubkey.Serialize(data_stream);

        CPubKey pubkey_deserialized;
        pubkey_deserialized.Unserialize(data_stream);
        CHECK(pubkey_deserialized == pubkey);
    }

    {
        const CScript tx_pubkey_script = GetScriptForRawPubKey(pubkey);
        CHECK(!tx_pubkey_script.IsPayToScriptHash());
        CHECK(!tx_pubkey_script.IsPayToWitnessScriptHash());
        CHECK(!tx_pubkey_script.IsPushOnly());
        CHECK(!tx_pubkey_script.IsUnspendable());
        CHECK(tx_pubkey_script.HasValidOps());
        CHECK(tx_pubkey_script.size() == 35);

        const CScript tx_multisig_script = GetScriptForMultisig(1, {pubkey});
        CHECK(!tx_multisig_script.IsPayToScriptHash());
        CHECK(!tx_multisig_script.IsPayToWitnessScriptHash());
        CHECK(!tx_multisig_script.IsPushOnly());
        CHECK(!tx_multisig_script.IsUnspendable());
        CHECK(tx_multisig_script.HasValidOps());
        CHECK(tx_multisig_script.size() == 37);

        FillableSigningProvider fillable_signing_provider;
        CHECK(!IsSegWitOutput(fillable_signing_provider, tx_pubkey_script));
        CHECK(!IsSegWitOutput(fillable_signing_provider, tx_multisig_script));
        CHECK(fillable_signing_provider.GetKeys().size() == 0);
        CHECK(!fillable_signing_provider.HaveKey(pubkey.GetID()));

        const bool ok_add_key = fillable_signing_provider.AddKey(key);
        CHECK(ok_add_key);
        CHECK(fillable_signing_provider.HaveKey(pubkey.GetID()));

        FillableSigningProvider fillable_signing_provider_pub;
        CHECK(!fillable_signing_provider_pub.HaveKey(pubkey.GetID()));

        const bool ok_add_key_pubkey = fillable_signing_provider_pub.AddKeyPubKey(key, pubkey);
        CHECK(ok_add_key_pubkey);
        CHECK(fillable_signing_provider_pub.HaveKey(pubkey.GetID()));

        TxoutType which_type_tx_pubkey;
        const bool is_standard_tx_pubkey = IsStandard(tx_pubkey_script, which_type_tx_pubkey);
        CHECK(is_standard_tx_pubkey);
        CHECK(which_type_tx_pubkey == TxoutType::PUBKEY);

        TxoutType which_type_tx_multisig;
        const bool is_standard_tx_multisig = IsStandard(tx_multisig_script, which_type_tx_multisig);
        CHECK(is_standard_tx_multisig);
        CHECK(which_type_tx_multisig == TxoutType::MULTISIG);

        std::vector<std::vector<unsigned char>> v_solutions_ret_tx_pubkey;
        const TxoutType outtype_tx_pubkey = Solver(tx_pubkey_script, v_solutions_ret_tx_pubkey);
        CHECK(outtype_tx_pubkey == TxoutType::PUBKEY);
        CHECK(v_solutions_ret_tx_pubkey.size() == 1);
        CHECK(v_solutions_ret_tx_pubkey[0].size() == 33);

        std::vector<std::vector<unsigned char>> v_solutions_ret_tx_multisig;
        const TxoutType outtype_tx_multisig = Solver(tx_multisig_script, v_solutions_ret_tx_multisig);
        CHECK(outtype_tx_multisig == TxoutType::MULTISIG);
        CHECK(v_solutions_ret_tx_multisig.size() == 3);
        CHECK(v_solutions_ret_tx_multisig[0].size() == 1);
        CHECK(v_solutions_ret_tx_multisig[1].size() == 33);
        CHECK(v_solutions_ret_tx_multisig[2].size() == 1);

        OutputType output_type{};
        const CTxDestination tx_destination{PKHash{pubkey}};
        CHECK(output_type == OutputType::LEGACY);
        CHECK(IsValidDestination(tx_destination));
        CHECK(PKHash{pubkey} == *std::get_if<PKHash>(&tx_destination));

        const CScript script_for_destination = GetScriptForDestination(tx_destination);
        CHECK(script_for_destination.size() == 25);

        const std::string destination_address = EncodeDestination(tx_destination);
        CHECK(DecodeDestination(destination_address) == tx_destination);

        CKeyID key_id = pubkey.GetID();
        CHECK(!key_id.IsNull());
        CHECK(key_id == CKeyID{key_id});
        CHECK(key_id == GetKeyForDestination(fillable_signing_provider, tx_destination));

        CPubKey pubkey_out;
        const bool ok_get_pubkey = fillable_signing_provider.GetPubKey(key_id, pubkey_out);
        CHECK(ok_get_pubkey);

        CKey key_out;
        const bool ok_get_key = fillable_signing_provider.GetKey(key_id, key_out);
        CHECK(ok_get_key);
        CHECK(fillable_signing_provider.GetKeys().size() == 1);
        CHECK(fillable_signing_provider.HaveKey(key_id));

        KeyOriginInfo key_origin_info;
        const bool ok_get_key_origin = fillable_signing_provider.GetKeyOrigin(key_id, key_origin_info);
        CHECK(!ok_get_key_origin);
    }

    {
        const std::vector<unsigned char> vch_pubkey{pubkey.begin(), pubkey.end()};
        CHECK(CPubKey::ValidSize(vch_pubkey));
        CHECK(!CPubKey::ValidSize({pubkey.begin(), pubkey.begin() + pubkey.size() - 1}));

        const CPubKey pubkey_ctor_1{vch_pubkey};
        CHECK(pubkey == pubkey_ctor_1);

        const CPubKey pubkey_ctor_2{vch_pubkey.begin(), vch_pubkey.end()};
        CHECK(pubkey == pubkey_ctor_2);

        CPubKey pubkey_set;
        pubkey_set.Set(vch_pubkey.begin(), vch_pubkey.end());
        CHECK(pubkey == pubkey_set);
    }

    {
        const CPubKey invalid_pubkey{};
        CHECK(!invalid_pubkey.IsValid());
        CHECK(!invalid_pubkey.IsFullyValid());
        CHECK(!(pubkey == invalid_pubkey));
        CHECK(pubkey != invalid_pubkey);
        CHECK(pubkey < invalid_pubkey);
    }

    {
        // Cover CPubKey's operator[](unsigned int pos)
        unsigned int sum = 0;
        for (size_t i = 0; i < pubkey.size(); ++i) {
            sum += pubkey[i];
        }
        CHECK(std::accumulate(pubkey.begin(), pubkey.end(), 0U) == sum);
    }

    {
        CPubKey decompressed_pubkey = pubkey;
        CHECK(decompressed_pubkey.IsCompressed());

        const bool ok = decompressed_pubkey.Decompress();
        CHECK(ok);
        CHECK(!decompressed_pubkey.IsCompressed());
        CHECK(decompressed_pubkey.size() == 65);
    }

    {
        std::vector<unsigned char> vch_sig;
        const bool ok = key.Sign(random_uint256, vch_sig, false);
        CHECK(ok);
        CHECK(pubkey.Verify(random_uint256, vch_sig));
        CHECK(CPubKey::CheckLowS(vch_sig));

        const std::vector<unsigned char> vch_invalid_sig{vch_sig.begin(), vch_sig.begin() + vch_sig.size() - 1};
        CHECK(!pubkey.Verify(random_uint256, vch_invalid_sig));
        CHECK(!CPubKey::CheckLowS(vch_invalid_sig));
    }

    {
        std::vector<unsigned char> vch_compact_sig;
        const bool ok_sign_compact = key.SignCompact(random_uint256, vch_compact_sig);
        CHECK(ok_sign_compact);

        CPubKey recover_pubkey;
        const bool ok_recover_compact = recover_pubkey.RecoverCompact(random_uint256, vch_compact_sig);
        CHECK(ok_recover_compact);
        CHECK(recover_pubkey == pubkey);
    }

    {
        CPubKey child_pubkey;
        ChainCode child_chaincode;
        const bool ok = pubkey.Derive(child_pubkey, child_chaincode, 0, random_uint256);
        CHECK(ok);
        CHECK(child_pubkey != pubkey);
        CHECK(child_pubkey.IsCompressed());
        CHECK(child_pubkey.IsFullyValid());
        CHECK(child_pubkey.IsValid());
        CHECK(child_pubkey.size() == 33);
        CHECK(child_chaincode != random_uint256);
    }

    const CPrivKey priv_key = key.GetPrivKey();

    {
        for (const bool skip_check : {true, false}) {
            CKey loaded_key;
            const bool ok = loaded_key.Load(priv_key, pubkey, skip_check);
            CHECK(ok);
            CHECK(key == loaded_key);
        }
    }
}

FUZZ_TARGET(ellswift_roundtrip, .init = initialize_key)
{
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};

    CKey key = ConsumePrivateKey(fdp, /*compressed=*/true);
    if (!key.IsValid()) return;

    auto ent32 = fdp.ConsumeBytes<std::byte>(32);
    ent32.resize(32);

    auto encoded_ellswift = key.EllSwiftCreate(ent32);
    auto decoded_pubkey = encoded_ellswift.Decode();

    uint256 hash{ConsumeUInt256(fdp)};
    std::vector<unsigned char> sig;
    key.Sign(hash, sig);
    CHECK(decoded_pubkey.Verify(hash, sig));
}

FUZZ_TARGET(bip324_ecdh, .init = initialize_key)
{
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};

    // We generate private key, k1.
    CKey k1 = ConsumePrivateKey(fdp, /*compressed=*/true);
    if (!k1.IsValid()) return;

    // They generate private key, k2.
    CKey k2 = ConsumePrivateKey(fdp, /*compressed=*/true);
    if (!k2.IsValid()) return;

    // We construct an ellswift encoding for our key, k1_ellswift.
    auto ent32_1 = fdp.ConsumeBytes<std::byte>(32);
    ent32_1.resize(32);
    auto k1_ellswift = k1.EllSwiftCreate(ent32_1);

    // They construct an ellswift encoding for their key, k2_ellswift.
    auto ent32_2 = fdp.ConsumeBytes<std::byte>(32);
    ent32_2.resize(32);
    auto k2_ellswift = k2.EllSwiftCreate(ent32_2);

    // They construct another (possibly distinct) ellswift encoding for their key, k2_ellswift_bad.
    auto ent32_2_bad = fdp.ConsumeBytes<std::byte>(32);
    ent32_2_bad.resize(32);
    auto k2_ellswift_bad = k2.EllSwiftCreate(ent32_2_bad);
    CHECK((ent32_2_bad == ent32_2) == (k2_ellswift_bad == k2_ellswift));

    // Determine who is who.
    bool initiating = fdp.ConsumeBool();

    // We compute our shared secret using our key and their public key.
    auto ecdh_secret_1 = k1.ComputeBIP324ECDHSecret(k2_ellswift, k1_ellswift, initiating);
    // They compute their shared secret using their key and our public key.
    auto ecdh_secret_2 = k2.ComputeBIP324ECDHSecret(k1_ellswift, k2_ellswift, !initiating);
    // Those must match, as everyone is behaving correctly.
    CHECK(ecdh_secret_1 == ecdh_secret_2);

    if (k1_ellswift != k2_ellswift) {
        // Unless the two keys are exactly identical, acting as the wrong party breaks things.
        auto ecdh_secret_bad = k1.ComputeBIP324ECDHSecret(k2_ellswift, k1_ellswift, !initiating);
        CHECK(ecdh_secret_bad != ecdh_secret_1);
    }

    if (k2_ellswift_bad != k2_ellswift) {
        // Unless both encodings created by them are identical, using the second one breaks things.
        auto ecdh_secret_bad = k1.ComputeBIP324ECDHSecret(k2_ellswift_bad, k1_ellswift, initiating);
        CHECK(ecdh_secret_bad != ecdh_secret_1);
    }
}
